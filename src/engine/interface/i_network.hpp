#pragma once

#include <coroutine>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace game {

// network.dll interface. Carries only bytes (protobuf/gzip are on the game side). TCP and UDP use the SAME port:
//  kReliable   -> TCP (guaranteed, ordered)
//  kUnreliable -> UDP (not guaranteed, unordered; may be lost)
enum class EDeliver : uint8_t { kReliable, kUnreliable };

enum class ENetEvent : uint8_t {
    kConnected,     // peer connected (on the client: handshake with the server finished)
    kDisconnected,  // peer left
    kPacket,        // data arrived
};

using PeerId_t = uint32_t;  // 0 is invalid
// On the client, the only remote end is the server; it is addressed with this id and it appears in events.
inline constexpr PeerId_t kServerPeer = 0xFFFFFFFFu;
// Packets larger than this can only be sent with kReliable (UDP is not fragmented here, so stay under the MTU).
inline constexpr std::size_t kMaxUnreliableSize = 1200;

struct NetEvent_t {
    ENetEvent type = ENetEvent::kPacket;
    PeerId_t peer = 0;
    EDeliver deliver = EDeliver::kReliable;
    std::vector<uint8_t> data;  // only for kPacket
};

class INetwork {
public:
    virtual ~INetwork() = default;

    // Server: listens on TCP on the given port + binds UDP to the same port.
    [[nodiscard]] virtual bool listen(uint16_t port) = 0;
    // Client: connects to the server over TCP (blocks), then switches UDP to the same port. The kConnected event arrives when done.
    [[nodiscard]] virtual bool connect(const std::string& host, uint16_t port) = 0;
    virtual void shutdown() = 0;
    virtual void disconnect(PeerId_t peer) = 0;
    // Client: whether there is a server connection. Always false on the server (clients are reported through events).
    [[nodiscard]] virtual bool connected() const = 0;

    // false: could not send (no peer, UDP not bound yet, packet too large for kUnreliable).
    [[nodiscard]] virtual bool send(PeerId_t peer, std::span<const uint8_t> data, EDeliver deliver) = 0;
    virtual void broadcast(std::span<const uint8_t> data, EDeliver deliver) = 0;

    // Reads/writes the sockets and queues events. Called once per tick; does not block.
    virtual void pump() = 0;
    [[nodiscard]] virtual bool hasEvent() const = 0;
    [[nodiscard]] virtual std::optional<NetEvent_t> nextEvent() = 0;
    // Client: own peer id after connect() (the server also uses it as the player id).
    [[nodiscard]] virtual PeerId_t localPeer() const = 0;

    // Coroutine support (see engine/interface/network_task.hpp): when an event arrives, pump() resumes this coroutine.
    // Only one waiter at a time.
    virtual void setWaiter(std::coroutine_handle<> waiter) = 0;
};

inline constexpr char kNetworkInterface[] = "Network001";

}  // namespace game
