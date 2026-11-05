#include "server/server_app.hpp"

#include "engine/engine/core/asset_path.hpp"
#include "engine/engine/core/config.hpp"
#include "engine/interface/i_log.hpp"
#include "game/server/world_tick.hpp"
#include "game/shared/level.hpp"

#include <algorithm>
#include <chrono>
#include <thread>
#include <tuple>

namespace game {

bool CServerApp::connect(CreateInterfaceFn factory) {
    INetwork* network = queryInterface<INetwork>(factory, kNetworkInterface);
    if (network == nullptr) {
        GAME_LOG_ERROR("server", "{} bulunamadı", kNetworkInterface);
        return false;
    }
    m_network.bind(network, &m_world);
    m_physics = queryInterface<IPhysics>(factory, kPhysicsInterface);
    if (m_physics == nullptr || !m_physics->initialize()) {
        GAME_LOG_ERROR("server", "{} bulunamadı veya başlatılamadı", kPhysicsInterface);
        return false;
    }

    // Sunucu ayarları: assets/config/server.cfg (dosya yoksa varsayılanlar).
    ConfigValues_t settings;
    if (const auto path = findAssetPath("config/server.cfg")) {
        settings = config::load(*path);
    }
    const bool playerCollisions = config::getBool(settings, "player.collisions.enabled", true);
    m_physics->setCharacterCollisions(playerCollisions);
    GAME_LOG_INFO("server", "player.collisions.enabled = {}", playerCollisions);
    m_botCount = std::max(0, config::getInt(settings, "bots.count", 0));
    GAME_LOG_INFO("server", "bots.count = {}", m_botCount);
    m_robotCount = std::max(0, config::getInt(settings, "robots.count", 1));
    GAME_LOG_INFO("server", "robots.count = {}", m_robotCount);
    m_dancerCount = std::max(0, config::getInt(settings, "dancers.count", 1));
    GAME_LOG_INFO("server", "dancers.count = {}", m_dancerCount);
    return true;
}

int CServerApp::run(const LaunchOptions_t& options) {
    world::reset(m_world, options.tickRate);
    m_world.physics = m_physics;
    level::buildPhysics(*m_physics);
    for (int i = 0; i < m_botCount; ++i) {
        std::ignore = world::spawnBot(m_world, {-3.0f - static_cast<float>(i) * 1.5f, 0.0f, -4.0f});
    }

    for (int i = 0; i < m_robotCount; ++i) {
        // Haritanın ortasında, spawn'ın önünde; sonraki robotlar yanına dizilir.
        std::ignore = world::spawnAnimated(m_world, EModel::kRobot, {static_cast<float>(i) * 2.0f, 0.0f, -4.0f});
    }
    for (int i = 0; i < m_dancerCount; ++i) {
        std::ignore = world::spawnAnimated(m_world, EModel::kDancer, {4.0f + static_cast<float>(i) * 2.0f, 0.0f, -5.0f});
    }

    if (!m_network.listen(options.port)) {
        return 1;
    }
    GAME_LOG_INFO("server", "port {}, tick rate {}", options.port, m_world.tickRate);

    using clock = std::chrono::steady_clock;
    const auto period = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(world::tickDelta(m_world)));
    auto nextTick = clock::now();

    while (true) {
        m_network.pump();
        world::tick(m_world);
        m_network.broadcastSnapshot();

        // Sabit tick aralığı. Geride kalırsak bir sonraki tick'e atla (spiral birikmesin).
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
