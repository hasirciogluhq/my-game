#pragma once

#include "engine/interface/i_network.hpp"
#include "engine/network/api.hpp"

#include <chrono>
#include <coroutine>
#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

namespace game {

// Socket handle (RAII). Platform differences are in network_system.cpp.
class CSocketFd {
public:
  CSocketFd() = default;
  explicit CSocketFd(int fd) : m_fd(fd) {}
  CSocketFd(const CSocketFd &) = delete;
  CSocketFd &operator=(const CSocketFd &) = delete;
  CSocketFd(CSocketFd &&other) noexcept : m_fd(other.release()) {}
  CSocketFd &operator=(CSocketFd &&other) noexcept;
  ~CSocketFd();

  [[nodiscard]] int get() const { return m_fd; }
  [[nodiscard]] bool valid() const { return m_fd >= 0; }
  int release();
  void reset();

private:
  int m_fd = -1;
};

// A connection to one end: TCP (kReliable) + the UDP path to that end (kUnreliable).
// Wire format: TCP = [u32 length][data]; UDP = [u32 token][data] (when data is empty it is a
// bind request/acknowledgement).
struct Connection_t {
  CSocketFd tcp;
  std::vector<uint8_t> in;  // incoming TCP bytes not yet split into frames
  std::vector<uint8_t> out; // outgoing TCP bytes not yet written
  uint32_t token =
      0; // identity of UDP packets; the server gives it during the TCP handshake
  uint32_t udpAddress = 0; // server side: the peer's UDP address (network byte order)
  uint16_t udpPort = 0;
  bool udpBound = false; // UDP path established (bind request acknowledged)
  bool broken = false; // write error or outgoing buffer full: pump() removes it
};

// A client on the server.
struct Client_t {
  PeerId_t id = 0;
  Connection_t connection;
};

// TCP + UDP, one port. Interface: INetwork (Network001).
//
// Client: if `m_connection` is set, it is connected to the server.
// Server: `m_clients` holds every client and its connection. It listens on TCP and binds UDP
// on the same port. Handshake: the server sends [peer id, token] in the first TCP frame of an
// accepted connection. The client sends the token over UDP (bind request); the server maps the
// UDP address and acknowledges it. After that, kUnreliable packets travel over UDP.
class NETWORK_API CNetworkSystem final : public INetwork {
public:
  CNetworkSystem() = default;
  ~CNetworkSystem() override;

  [[nodiscard]] bool listen(uint16_t port) override;
  [[nodiscard]] bool connect(const std::string &host, uint16_t port) override;
  void shutdown() override;
  void disconnect(PeerId_t peer) override;
  [[nodiscard]] bool connected() const override {
    return m_connection.has_value();
  }

  [[nodiscard]] bool send(PeerId_t peer, std::span<const uint8_t> data,
                          EDeliver deliver) override;
  void broadcast(std::span<const uint8_t> data, EDeliver deliver) override;

  void pump() override;
  [[nodiscard]] bool hasEvent() const override { return !m_events.empty(); }
  [[nodiscard]] std::optional<NetEvent_t> nextEvent() override;
  [[nodiscard]] PeerId_t localPeer() const override { return m_localPeer; }

  void setWaiter(std::coroutine_handle<> waiter) override { m_waiter = waiter; }

private:
  [[nodiscard]] Connection_t *findConnection(PeerId_t peer);
  void acceptClients();
  // Reads and writes one connection. false: the connection dropped or broke.
  [[nodiscard]] bool service(PeerId_t peer, Connection_t &connection);
  [[nodiscard]] bool readTcp(PeerId_t peer, Connection_t &connection);
  [[nodiscard]] bool extractFrames(PeerId_t peer, Connection_t &connection);
  void onFrame(PeerId_t peer, Connection_t &connection,
               std::span<const uint8_t> frame);
  void readUdp();
  void onDatagram(uint32_t token, std::span<const uint8_t> payload,
                  uint32_t fromAddress, uint16_t fromPort);
  void sendBindRequest();
  [[nodiscard]] bool sendReliable(Connection_t &connection,
                                  std::span<const uint8_t> data);
  [[nodiscard]] bool sendUnreliable(const Connection_t &connection,
                                    std::span<const uint8_t> data);
  // Sends a [token][data] datagram (server: to the connection's UDP address, client: to the server).
  [[nodiscard]] bool writeDatagram(const Connection_t &connection,
                                   std::span<const uint8_t> payload);
  [[nodiscard]] bool flush(Connection_t &connection);
  void dropClient(PeerId_t id);
  void dropConnection();
  void pushEvent(NetEvent_t event);
  void wakeWaiter();
  [[nodiscard]] uint32_t makeToken() const;

  // Server
  CSocketFd m_listener;
  std::unordered_map<PeerId_t, Client_t> m_clients;
  std::unordered_map<uint32_t, PeerId_t> m_tokens; // UDP token -> client
  PeerId_t m_nextPeer = 1;
  // Client
  std::optional<Connection_t> m_connection;
  PeerId_t m_localPeer = 0; // 0 until the handshake completes
  std::chrono::steady_clock::time_point m_lastBindRequest{};
  // Common
  CSocketFd m_udp;
  std::deque<NetEvent_t> m_events;
  std::coroutine_handle<> m_waiter;
};

} // namespace game
