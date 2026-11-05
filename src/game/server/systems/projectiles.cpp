#include "game/server/systems/projectiles.hpp"

#include "engine/interface/i_log.hpp"
#include "game/shared/movement.hpp"
#include "game/shared/trace.hpp"

#include <algorithm>
#include <vector>

#include <glm/geometric.hpp>

namespace game::systems {
namespace {

// Sağlık 0'ın altına inmez. Ölümü systems::death halleder.
void applyDamage(World_t& world, entt::entity victim, uint32_t attackerNetId, uint32_t damage) {
    auto& player = world.registry.get<Player_t>(victim);
    player.health -= std::min(player.health, damage);
    GAME_LOG_DEBUG("combat", "pawn {} pawn {} tarafından vuruldu: -{} (hp {})", world.registry.get<Replicated_t>(victim).netId,
                   attackerNetId, damage, player.health);
}

}  // namespace

void projectiles(World_t& world) {
    const float dt = world::tickDelta(world);

    std::vector<entt::entity> dead;
    for (auto [entity, projectile] : world.registry.view<Projectile_t>().each()) {
        const float age = static_cast<float>(world.tick - projectile.spawnTick) * dt;

        // Bu tick'ten bir sonrakine giden doğru parçasında dünyaya ya da bir oyuncuya çarpıyor mu?
        // Ateş eden pawn atlanır (mermi göz hizasından çıkar).
        const glm::vec3 from = projectilePosition(projectile, world.tick, dt);
        const glm::vec3 step = projectilePosition(projectile, world.tick + 1, dt) - from;
        const float length = glm::length(step);
        bool hitSomething = false;
        if (length > 0.0f) {
            const trace::TraceHit_t hit = trace::line(world, {.origin = from,
                                                              .direction = step / length,
                                                              .maxDistance = length,
                                                              .ignore = world::findByNetId(world, projectile.ownerId)});
            hitSomething = hit.kind != trace::ETraceKind::kNone;
            if (hit.kind == trace::ETraceKind::kPlayer) {
                applyDamage(world, hit.entity, projectile.ownerId, kProjectileDamage);
            }
        }
        if (age > kProjectileLifetime || hitSomething) {
            dead.push_back(entity);
        }
    }
    for (const entt::entity entity : dead) {
        world::destroy(world, entity);
    }
}

}  // namespace game::systems
