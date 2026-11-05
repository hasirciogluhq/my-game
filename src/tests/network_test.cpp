// Başsız ağ testi: aynı portta TCP + UDP, el sıkışma, kReliable / kUnreliable, coroutine.
// Çalıştır: xmake run network_test
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

// Her iki ucu da pump eder; bitene kadar (ya da süre dolana kadar) bekler.
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

// Server tarafı coroutine: her olayı sırayla sayar.
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

    check(server.listen(kTestPort), "server: TCP+UDP aynı portta dinliyor");
    serve(server, log);  // coroutine başlar, ilk olayı bekler
    check(client.connect("127.0.0.1", kTestPort), "client: bağlandı");
    check(client.connected(), "client: connected()");

    check(waitUntil(server, client, [&] { return log.connects == 1 && client.localPeer() != 0; }), "el sıkışma: peer id + token");
    const PeerId_t peer = client.localPeer();

    // Client -> server, güvenilir (TCP). Coroutine server tarafında devam eder.
    check(client.send(kServerPeer, bytesOf("merhaba"), EDeliver::kReliable), "client: reliable gönderdi");
    check(waitUntil(server, client, [&] { return log.reliable == 1; }), "server coroutine: reliable geldi");
    check(log.lastReliable == "merhaba", "reliable içerik doğru");

    // UDP yolu kurulana kadar bekle (bind isteği/onayı).
    check(waitUntil(server, client, [&] { return client.send(kServerPeer, bytesOf("udp-1"), EDeliver::kUnreliable); }),
          "UDP bind: client unreliable gönderebiliyor");
    check(waitUntil(server, client, [&] { return log.unreliable >= 1; }), "server: unreliable (UDP) geldi");
    check(log.lastUnreliable == "udp-1", "unreliable içerik doğru");

    // Server -> client: önce UDP, sonra TCP.
    bool gotUdp = false;
    bool gotTcp = false;
    check(server.send(peer, bytesOf("snap"), EDeliver::kUnreliable), "server: unreliable gönderdi");
    check(server.send(peer, bytesOf("welcome"), EDeliver::kReliable), "server: reliable gönderdi");
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
          "client: UDP snapshot + TCP welcome geldi");

    // Büyük paket: kReliable gider, kUnreliable reddedilir.
    const std::string big(kMaxUnreliableSize + 1, 'x');
    check(!client.send(kServerPeer, bytesOf(big), EDeliver::kUnreliable), "büyük paket UDP'den reddedildi");
    check(client.send(kServerPeer, bytesOf(big), EDeliver::kReliable), "büyük paket TCP'den gitti");
    check(waitUntil(server, client, [&] { return log.reliable == 2; }), "server: büyük reliable paket geldi");
    check(log.lastReliable.size() == big.size(), "büyük paket boyutu doğru");

    // Kopma: client kapanınca server kDisconnected görür.
    client.shutdown();
    check(waitUntil(server, client, [&] { return log.disconnects == 1; }), "server: client ayrılınca kDisconnected");

    std::cout << (g_failures == 0 ? "TUM TESTLER GECTI\n" : "BAZI TESTLER BASARISIZ\n");
    return g_failures == 0 ? 0 : 1;
}
