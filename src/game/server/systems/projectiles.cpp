#include "game/server/systems/projectiles.hpp"

#include "engine/interface/i_log.hpp"
#include "game/shared/movement.hpp"
#include "game/shared/trace.hpp"

#include <algorithm>
#include <vector>

#include <glm/geometric.hpp>

namespace game::systems {
namespace {

// Health does not go below 0. systems::death handles the death.
void applyDamage(World_t& world, entt::entity victim, uint32_t attackerNetId, uint32_t damage) {
    auto& player = world.registry.get<Player_t>(victim);
    player.health -= std::min(player.health, damage);
    GAME_LOG_DEBUG("combat", "pawn {} was hit by pawn {}: -{} (hp {})", world.registry.get<Replicated_t>(victim).netId,
                   attackerNetId, damage, player.health);
}

}  // namespace

void projectiles(World_t& world) {
    const float dt = world::tickDelta(world);

    std::vector<entt::entity> dead;
    for (auto [entity, projectile] : world.registry.view<Projectile_t>().each()) {
        const float age = static_cast<float>(world.tick - projectile.spawnTick) * dt;

        // Does it hit the world or a player on the straight segment from this tick to the next?
        // The firing pawn is skipped (the bullet leaves from eye height).
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
