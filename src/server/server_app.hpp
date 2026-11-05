#pragma once

#include "engine/engine/core/launch_options.hpp"
#include "engine/interface/i_physics.hpp"
#include "game/shared/world.hpp"
#include "server/api.hpp"
#include "server/i_server.hpp"
#include "server/net/net_server.hpp"

namespace game {

// Headless authoritative server container. Each tick: network.pump (connection events + commands) -> world::tick
// -> snapshot broadcast. The entity list is `m_world` (the ECS registry, as on the client); connections are `m_network`.
// Physics comes from physics.dll. Game rules live under game/server/.
class SERVER_API CServerApp final : public IServerApp {
public:
    [[nodiscard]] bool connect(CreateInterfaceFn factory) override;
    [[nodiscard]] bool open(const LaunchOptions_t& options) override;
    void loop() override;

private:
    IPhysics* m_physics = nullptr;  // own physics world (IPhysicsFactory::createWorld)
    World_t m_world;                // the entity list: everything on the server
    CNetServer m_network;           // own network instance (INetworkFactory::createNetwork)
    MovementSettings_t m_movement;  // move.* cvars from server.cfg; copied into m_world in open()
    EMap m_map = EMap::kArena;      // map.name from server.cfg
    int m_botCount = 0;
    float m_botTurnRate = 0.6f;
};

}  // namespace game
