#pragma once

#include <coroutine>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace game {

// network.dll arayüzü. Sadece bayt taşır (protobuf/gzip oyun tarafında). TCP ve UDP AYNI portu kullanır:
//  kReliable   -> TCP (garantili, sıralı)
//  kUnreliable -> UDP (garantisiz, sırasız; kayıp olabilir)
enum class EDeliver : uint8_t { kReliable, kUnreliable };

enum class ENetEvent : uint8_t {
    kConnected,     // peer bağlandı (client'ta: server ile el sıkışma bitti)
    kDisconnected,  // peer gitti
    kPacket,        // veri geldi
};

using PeerId_t = uint32_t;  // 0 geçersiz
// Client tarafında tek karşı uç server'dır; ona bu id ile gönderilir ve olaylarda bu id görünür.
inline constexpr PeerId_t kServerPeer = 0xFFFFFFFFu;
// Bundan büyük paket kReliable dışında gönderilemez (UDP parçalamıyoruz, MTU altında kal).
inline constexpr std::size_t kMaxUnreliableSize = 1200;

struct NetEvent_t {
    ENetEvent type = ENetEvent::kPacket;
    PeerId_t peer = 0;
    EDeliver deliver = EDeliver::kReliable;
    std::vector<uint8_t> data;  // sadece kPacket
};

class INetwork {
public:
    virtual ~INetwork() = default;

    // Server: aynı numaralı portta TCP dinler + UDP bağlar.
    [[nodiscard]] virtual bool listen(uint16_t port) = 0;
    // Client: server'a TCP bağlanır (bloklar), UDP'yi aynı porta çevirir. Bitince kConnected olayı gelir.
    [[nodiscard]] virtual bool connect(const std::string& host, uint16_t port) = 0;
    virtual void shutdown() = 0;
    virtual void disconnect(PeerId_t peer) = 0;
    // Client: server bağlantısı var mı. Server'da her zaman false (client'ları olaylar bildirir).
    [[nodiscard]] virtual bool connected() const = 0;

    // false: gönderilemedi (peer yok, UDP henüz bağlı değil, kUnreliable için paket çok büyük).
    [[nodiscard]] virtual bool send(PeerId_t peer, std::span<const uint8_t> data, EDeliver deliver) = 0;
    virtual void broadcast(std::span<const uint8_t> data, EDeliver deliver) = 0;

    // Soketleri okur/yazar, olayları kuyruğa koyar. Her tick bir kez çağrılır, bloklamaz.
    virtual void pump() = 0;
    [[nodiscard]] virtual bool hasEvent() const = 0;
    [[nodiscard]] virtual std::optional<NetEvent_t> nextEvent() = 0;
    // client: connect() sonrası kendi peer id'si (server bunu oyuncu id'si olarak da kullanır).
    [[nodiscard]] virtual PeerId_t localPeer() const = 0;

    // Coroutine desteği (bkz. engine/interface/network_task.hpp): olay gelince pump() bu coroutine'i devam ettirir.
    // Aynı anda tek bekleyen olabilir.
    virtual void setWaiter(std::coroutine_handle<> waiter) = 0;
};

inline constexpr char kNetworkInterface[] = "Network001";

}  // namespace game
