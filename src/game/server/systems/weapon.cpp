#include "game/server/systems/weapon.hpp"

#include "game/server/control/pawn_command.hpp"
#include "game/shared/movement.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace game::systems {

void weapon(World_t& world) {
    const auto cooldownTicks = std::max(1u, static_cast<uint32_t>(std::ceil(kFireInterval * static_cast<float>(world.tickRate))));

    std::vector<Projectile_t> spawns;
    for (auto [pawn, transform, gun] : world.registry.view<Transform_t, Weapon_t>().each()) {
        if (!hasButton(control_rules::commandOf(world, pawn).buttons, EButton::kAttack) || world.tick < gun.nextFireTick) {
            continue;
        }
        gun.nextFireTick = world.tick + cooldownTicks;
        spawns.push_back({.ownerId = world.registry.get<Replicated_t>(pawn).netId,
                          .spawnTick = world.tick,
                          .origin = transform.position + glm::vec3{0.0f, kEyeHeight, 0.0f},
                          .velocity = lookDirection(transform) * kProjectileSpeed});
    }

    // Registry iterasyonu bittikten sonra yarat.
    for (const Projectile_t& projectile : spawns) {
        const entt::entity entity = world::createReplicated(world);
        world.registry.emplace<Transform_t>(entity, projectile.origin);
        world.registry.emplace<Projectile_t>(entity, projectile);
    }
}

}  // namespace game::systems
