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
    [[nodiscard]] int run(const LaunchOptions_t& options) override;

private:
    IPhysics* m_physics = nullptr;
    World_t m_world;  // the entity list: everything on the server
    CNetServer m_network;
    MovementSettings_t m_movement;  // move.* cvars from server.cfg; copied into m_world in run()
    int m_botCount = 0;
    float m_botTurnRate = 0.6f;
};

}  // namespace game
