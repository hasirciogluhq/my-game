#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// One player, one command: server movement and client prediction use the same code.
void movePlayer(World_t& world, entt::entity entity, const UserCmd_t& cmd);

// On the client, every frame: computes the projectile's Transform from the projection formula.
// atTick may be fractional (the display time of interpolation); the server gives world.tick.
void projectileTransforms(World_t& world, double atTick);

// pos(t) = origin + velocity * (t - spawnTick) * dt   (if t < spawnTick, the muzzle point)
[[nodiscard]] glm::vec3 projectilePosition(const Projectile_t& projectile, double atTick, float tickDelta);

// Look direction: pitch up/down, yaw in the horizontal plane. At yaw=0 it is -Z.
[[nodiscard]] glm::vec3 lookDirection(const Transform_t& transform);

}  // namespace game::systems
