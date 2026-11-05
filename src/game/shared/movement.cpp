#include "game/shared/movement.hpp"

#include "engine/engine/core/camera.hpp"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace game::systems {

glm::vec3 lookDirection(const Transform_t& transform) {
    return {std::cos(transform.pitch) * std::sin(transform.yaw), std::sin(transform.pitch),
            -std::cos(transform.pitch) * std::cos(transform.yaw)};
}

glm::vec3 projectilePosition(const Projectile_t& projectile, double atTick, float tickDelta) {
    // Before its spawn time (interpolation delay), the projectile stays at the muzzle point.
    const double elapsedTicks = std::max(0.0, atTick - static_cast<double>(projectile.spawnTick));
    return projectile.origin + projectile.velocity * (static_cast<float>(elapsedTicks) * tickDelta);
}

void movePlayer(World_t& world, entt::entity entity, const UserCmd_t& cmd) {
    auto [transform, velocity, player] = world.registry.get<Transform_t, Velocity_t, Player_t>(entity);
    const float dt = world::tickDelta(world);

    // Do not trust the view angles coming from the client: clamp them.
    transform.yaw = cmd.yaw;
    transform.pitch = cmd.pitch;
    clampViewAngles(transform.yaw, transform.pitch);

    // At yaw=0: forward = -Z, right = +X.
    const glm::vec3 forward{std::sin(transform.yaw), 0.0f, -std::cos(transform.yaw)};
    const glm::vec3 right{std::cos(transform.yaw), 0.0f, std::sin(transform.yaw)};
    glm::vec3 wish = forward * cmd.forwardMove + right * cmd.sideMove;
    if (glm::length(wish) > 1.0f) {
        wish = glm::normalize(wish);
    }

    float speed = hasButton(cmd.buttons, EButton::kWalk) ? kWalkSpeed : kRunSpeed;
    if (hasButton(cmd.buttons, EButton::kCrouch)) {
        speed *= 0.5f;
    }
    velocity.linear.x = wish.x * speed;
    velocity.linear.z = wish.z * speed;

    if (player.onGround && hasButton(cmd.buttonsPressed, EButton::kJump)) {
        velocity.linear.y = kJumpSpeed;
    }
    velocity.linear.y -= kGravity * dt;  // every tick: ground contact comes back from physics as onGround

    if (const auto* collider = world.registry.try_get<Collider_t>(entity); collider != nullptr && world.physics != nullptr) {
        const CharacterResult_t result = world.physics->moveCharacter(collider->handle, velocity.linear, dt);
        transform.position = result.position;
        player.onGround = result.onGround;
    } else {
        // No physics module: only the y=0 ground.
        transform.position += velocity.linear * dt;
        player.onGround = transform.position.y <= 0.0f;
        transform.position.y = std::max(transform.position.y, 0.0f);
    }
    if (player.onGround && velocity.linear.y < 0.0f) {
        velocity.linear.y = 0.0f;
    }
}

void projectileTransforms(World_t& world, double atTick) {
    const float dt = world::tickDelta(world);
    for (auto [entity, transform, projectile] : world.registry.view<Transform_t, const Projectile_t>().each()) {
        transform.position = projectilePosition(projectile, atTick, dt);
    }
}

}  // namespace game::systems
