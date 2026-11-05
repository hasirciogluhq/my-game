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

// Soket tanıtıcısı (RAII). Platform farkları network_system.cpp'de.
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

// Bir uçla olan bağlantı: TCP (kReliable) + o uca giden UDP yolu (kUnreliable).
// Tel formatı: TCP = [u32 uzunluk][veri]; UDP = [u32 token][veri] (veri boşsa
// bind isteği/onayı).
struct Connection_t {
  CSocketFd tcp;
  std::vector<uint8_t> in;  // henüz çerçevelenmemiş gelen TCP baytları
  std::vector<uint8_t> out; // henüz yazılamayan giden TCP baytları
  uint32_t token =
      0; // UDP paketlerinin kimliği; TCP el sıkışmasında server verir
  uint32_t udpAddress = 0; // server tarafı: karşı ucun UDP adresi (ağ sırası)
  uint16_t udpPort = 0;
  bool udpBound = false; // UDP yolu kuruldu (bind isteği onaylandı)
  bool broken = false; // yazma hatası ya da giden tampon doldu: pump() kaldırır
};

// Server'ın bir client'ı.
struct Client_t {
  PeerId_t id = 0;
  Connection_t connection;
};

// TCP + UDP, tek port. Arayüz: INetwork (Network001).
//
// Client: `m_connection` doluysa server'a bağlıdır.
// Server: `m_clients` her client'ı ve onun bağlantısını tutar. Aynı portta TCP
// dinler + UDP bağlar. El sıkışma: server kabul ettiği bağlantıya ilk TCP
// çerçevesinde [peer id, token] yollar. Client UDP'den token'ı gönderir (bind
// isteği), server UDP adresini eşler ve onaylar; sonra kUnreliable paketler
// UDP'den gider.
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
  // Bir bağlantıyı okur ve yazar. false: bağlantı koptu ya da bozuldu.
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
  // [token][veri] datagramını yollar (server: bağlantının UDP adresine, client: server'a).
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
  PeerId_t m_localPeer = 0; // el sıkışma bitene kadar 0
  std::chrono::steady_clock::time_point m_lastBindRequest{};
  // Ortak
  CSocketFd m_udp;
  std::deque<NetEvent_t> m_events;
  std::coroutine_handle<> m_waiter;
};

} // namespace game
