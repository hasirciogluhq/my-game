#pragma once

#include "engine/interface/i_network.hpp"
#include "engine/interface/network_task.hpp"
#include "game/shared/world.hpp"
#include "server/api.hpp"

namespace game {

// Server'ın network yöneticisi (client'taki CNetClient'ın karşılığı): network.dll'in (INetwork) üstünde
// oyuncuların bağlantılarını tutar (peer id = player id), paketleri ilgili world çağrısına yönlendirir
// ve welcome / view target / snapshot gönderir. Oyun kuralı içermez (kurallar game/server/).
class SERVER_API CNetServer {
public:
    void bind(INetwork* network, World_t* world) {
        m_network = network;
        m_world = world;
    }
    [[nodiscard]] bool listen(uint16_t port);
    void pump();               // network.dll olaylarını getirir; coroutine uyanıp işler
    void broadcastSnapshot();  // dünyanın tam durumu, her tick

private:
    // Ağ olaylarını sırayla işleyen coroutine: pump() olay getirince uyanır.
    net::Task serveNetwork();
    void onPacket(const NetEvent_t& event);
    void sendWelcome(PeerId_t peer);
    void sendViewTarget(PeerId_t peer);

    INetwork* m_network = nullptr;
    World_t* m_world = nullptr;
};

}  // namespace game
