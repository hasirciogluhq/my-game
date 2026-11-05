#include "server/server_app.hpp"

#include "engine/engine/core/asset_path.hpp"
#include "engine/engine/core/config.hpp"
#include "engine/interface/i_log.hpp"
#include "game/server/map_rules.hpp"
#include "game/server/world_tick.hpp"
#include "game/shared/level.hpp"

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <tuple>

namespace game {

bool CServerApp::connect(CreateInterfaceFn factory) {
    auto* networkFactory = queryInterface<INetworkFactory>(factory, kNetworkFactoryInterface);
    INetwork* network = networkFactory != nullptr ? networkFactory->createNetwork() : nullptr;
    if (network == nullptr) {
        GAME_LOG_ERROR("server", "{} not found", kNetworkFactoryInterface);
        return false;
    }
    m_network.bind(network, &m_world);

    auto* physicsFactory = queryInterface<IPhysicsFactory>(factory, kPhysicsFactoryInterface);
    m_physics = physicsFactory != nullptr ? physicsFactory->createWorld() : nullptr;
    if (m_physics == nullptr || !m_physics->initialize()) {
        GAME_LOG_ERROR("server", "{} not found or could not be started", kPhysicsFactoryInterface);
        return false;
    }

    // Server settings: assets/config/server.cfg (defaults if the file is missing).
    ConfigValues_t settings;
    if (const auto path = findAssetPath("config/server.cfg")) {
        settings = config::load(*path);
    }
    const std::string mapText = config::getString(settings, "map.name", "arena");
    const std::optional<EMap> map = mapFromName(mapText);
    if (!map) {
        GAME_LOG_ERROR("server", "map.name '{}' is not a known map", mapText);
        return false;
    }
    m_map = *map;
    GAME_LOG_INFO("server", "map.name = {}", mapName(m_map));

    const bool playerCollisions = config::getBool(settings, "player.collisions.enabled", true);
    m_physics->setCharacterCollisions(playerCollisions);
    GAME_LOG_INFO("server", "player.collisions.enabled = {}", playerCollisions);
    m_movement = movementSettingsFrom(settings);
    m_botCount = std::max(0, config::getInt(settings, "bots.count", 0));
    GAME_LOG_INFO("server", "bots.count = {}", m_botCount);
    m_botTurnRate = config::getFloat(settings, "bots.turn_rate", 0.6f);
    GAME_LOG_INFO("server", "bots.turn_rate = {}", m_botTurnRate);
    return true;
}

bool CServerApp::open(const LaunchOptions_t& options) {
    world::reset(m_world, options.tickRate);
    m_world.physics = m_physics;
    m_world.movement = m_movement;
    m_world.map = m_map;
    level::buildPhysics(*m_physics, m_map);
    maprules::setup(m_world, options.testScene);  // map content and rules (may change m_world.movement)
    GAME_LOG_INFO("server", "move.run.speed = {}, move.slide.enabled = {}, move.bunnyhop.enabled = {}", m_world.movement.runSpeed,
                  m_world.movement.slide, m_world.movement.bunnyhop);
    for (int i = 0; i < m_botCount; ++i) {
        std::ignore = world::spawnBot(m_world, {-3.0f - static_cast<float>(i) * 1.5f, 0.0f, -4.0f}, m_botTurnRate);
    }

    if (!m_network.listen(options.port)) {
        return false;
    }
    GAME_LOG_INFO("server", "map {}, port {}, tick rate {}", mapName(m_world.map), options.port, m_world.tickRate);
    return true;
}

void CServerApp::loop() {
    using clock = std::chrono::steady_clock;
    const auto period = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(world::tickDelta(m_world)));
    auto nextTick = clock::now();

    while (true) {
        m_network.pump();
        world::tick(m_world);
        m_network.broadcastSnapshot();

        // Fixed tick interval. If we fall behind, skip to the next tick (no spiral build-up).
        nextTick += period;
        const auto now = clock::now();
        if (nextTick > now) {
            std::this_thread::sleep_until(nextTick);
        } else {
            nextTick = now;
        }
    }
}

}  // namespace game
