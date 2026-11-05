// Headless network test: TCP + UDP on the same port, handshake, kReliable / kUnreliable, coroutine.
// Run: xmake run network_test
#include "engine/interface/network_task.hpp"
#include "engine/network/network_system.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

namespace {

using namespace game;

int g_failures = 0;
constexpr uint16_t kTestPort = 47777;

void check(bool ok, const char* what) {
    std::cout << (ok ? "ok    " : "FAIL  ") << what << "\n";
    if (!ok) {
        ++g_failures;
    }
}

std::span<const uint8_t> bytesOf(std::string_view text) {
    return {reinterpret_cast<const uint8_t*>(text.data()), text.size()};
}

std::string textOf(const NetEvent_t& event) {
    return {event.data.begin(), event.data.end()};
}

// Pumps both ends; waits until done (or until the time runs out).
template <typename Done>
bool waitUntil(CNetworkSystem& a, CNetworkSystem& b, Done done) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
        a.pump();
        b.pump();
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
}

// Server-side coroutine: counts each event in order.
struct ServerLog_t {
    int connects = 0;
    int disconnects = 0;
    int reliable = 0;
    int unreliable = 0;
    std::string lastReliable;
    std::string lastUnreliable;
};

net::Task serve(INetwork& network, ServerLog_t& log) {
    while (true) {
        const NetEvent_t event = co_await net::nextEvent(network);
        if (event.type == ENetEvent::kConnected) {
            ++log.connects;
        } else if (event.type == ENetEvent::kDisconnected) {
            ++log.disconnects;
        } else if (event.type == ENetEvent::kPacket && event.deliver == EDeliver::kReliable) {
            ++log.reliable;
            log.lastReliable = textOf(event);
        } else if (event.type == ENetEvent::kPacket) {
            ++log.unreliable;
            log.lastUnreliable = textOf(event);
        }
    }
}

}  // namespace

int main() {
    CNetworkSystem server;
    CNetworkSystem client;
    ServerLog_t log;

    check(server.listen(kTestPort), "server: TCP+UDP listening on the same port");
    serve(server, log);  // the coroutine starts and waits for the first event
    check(client.connect("127.0.0.1", kTestPort), "client: connected");
    check(client.connected(), "client: connected()");

    check(waitUntil(server, client, [&] { return log.connects == 1 && client.localPeer() != 0; }), "handshake: peer id + token");
    const PeerId_t peer = client.localPeer();

    // Client -> server, reliable (TCP). The coroutine continues on the server side.
    check(client.send(kServerPeer, bytesOf("hello"), EDeliver::kReliable), "client: sent reliable");
    check(waitUntil(server, client, [&] { return log.reliable == 1; }), "server coroutine: reliable arrived");
    check(log.lastReliable == "hello", "reliable content correct");

    // Wait until the UDP path is set up (bind request/ack).
    check(waitUntil(server, client, [&] { return client.send(kServerPeer, bytesOf("udp-1"), EDeliver::kUnreliable); }),
          "UDP bind: client can send unreliable");
    check(waitUntil(server, client, [&] { return log.unreliable >= 1; }), "server: unreliable (UDP) arrived");
    check(log.lastUnreliable == "udp-1", "unreliable content correct");

    // Server -> client: UDP first, then TCP.
    bool gotUdp = false;
    bool gotTcp = false;
    check(server.send(peer, bytesOf("snap"), EDeliver::kUnreliable), "server: sent unreliable");
    check(server.send(peer, bytesOf("welcome"), EDeliver::kReliable), "server: sent reliable");
    check(waitUntil(server, client,
                    [&] {
                        while (auto event = client.nextEvent()) {
                            if (event->type == ENetEvent::kPacket) {
                                gotUdp = gotUdp || (event->deliver == EDeliver::kUnreliable && textOf(*event) == "snap");
                                gotTcp = gotTcp || (event->deliver == EDeliver::kReliable && textOf(*event) == "welcome");
                            }
                        }
                        return gotUdp && gotTcp;
                    }),
          "client: UDP snapshot + TCP welcome arrived");

    // Big packet: kReliable goes through, kUnreliable is rejected.
    const std::string big(kMaxUnreliableSize + 1, 'x');
    check(!client.send(kServerPeer, bytesOf(big), EDeliver::kUnreliable), "big packet rejected over UDP");
    check(client.send(kServerPeer, bytesOf(big), EDeliver::kReliable), "big packet sent over TCP");
    check(waitUntil(server, client, [&] { return log.reliable == 2; }), "server: big reliable packet arrived");
    check(log.lastReliable.size() == big.size(), "big packet size correct");

    // Disconnect: when the client closes, the server sees kDisconnected.
    client.shutdown();
    check(waitUntil(server, client, [&] { return log.disconnects == 1; }), "server: kDisconnected when the client leaves");

    std::cout << (g_failures == 0 ? "ALL TESTS PASSED\n" : "SOME TESTS FAILED\n");
    return g_failures == 0 ? 0 : 1;
}
