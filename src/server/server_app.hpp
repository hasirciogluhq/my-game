#pragma once

#include "engine/engine/core/launch_options.hpp"
#include "engine/interface/i_physics.hpp"
#include "game/shared/world.hpp"
#include "server/api.hpp"
#include "server/i_server.hpp"
#include "server/net/net_server.hpp"

namespace game {

// Headless otoriter server container. Her tick: network.pump (bağlantı olayları + komutlar) -> world::tick
// -> snapshot yayını. Entity listesi `m_world` (client'taki gibi ECS registry); bağlantılar `m_network`.
// Fizik physics.dll'den gelir. Oyun kuralları game/server/ altında.
class SERVER_API CServerApp final : public IServerApp {
public:
    [[nodiscard]] bool connect(CreateInterfaceFn factory) override;
    [[nodiscard]] int run(const LaunchOptions_t& options) override;

private:
    IPhysics* m_physics = nullptr;
    World_t m_world;  // entity listesi: server'daki her şey
    CNetServer m_network;
    int m_botCount = 0;
    int m_robotCount = 1;
};

}  // namespace game
