#include "game/shared/movement.hpp"

#include "engine/engine/core/camera.hpp"
#include "game/shared/movement_settings.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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

namespace {

constexpr float kNoCap = std::numeric_limits<float>::max();
constexpr float kStopEpsilon = 0.1f;  // m/s: below this the ground friction stops the player dead

// What the player asks for this tick: a unit direction and a target speed (0 when there is no input).
struct Wish_t {
    glm::vec3 dir{0.0f};
    float speed = 0.0f;
};

Wish_t wishFrom(const UserCmd_t& cmd, const MovementSettings_t& settings) {
    // At yaw=0: forward = -Z, right = +X.
    const glm::vec3 forward{std::sin(cmd.yaw), 0.0f, -std::cos(cmd.yaw)};
    const glm::vec3 right{std::cos(cmd.yaw), 0.0f, std::sin(cmd.yaw)};
    const glm::vec3 raw = forward * cmd.forwardMove + right * cmd.sideMove;
    const float length = glm::length(raw);
    if (length < kStopEpsilon) {
        return {};
    }

    float top = hasButton(cmd.buttons, EButton::kWalk) ? settings.walkSpeed : settings.runSpeed;
    if (hasButton(cmd.buttons, EButton::kCrouch)) {
        top *= settings.crouchScale;
    }
    return {.dir = raw / length, .speed = top * std::min(length, 1.0f)};
}

float horizontalSpeed(const glm::vec3& velocity) {
    return glm::length(glm::vec2{velocity.x, velocity.z});
}

void clampHorizontal(glm::vec3& velocity, float maxSpeed) {
    const float speed = horizontalSpeed(velocity);
    if (speed > maxSpeed) {
        velocity.x *= maxSpeed / speed;
        velocity.z *= maxSpeed / speed;
    }
}

// Source-style acceleration: only the speed gained along wishDir counts. addCap limits the speed that is
// asked for (air strafing: the cap is what makes strafe-jumping work).
void accelerate(glm::vec3& velocity, const Wish_t& wish, float accel, float addCap, float dt) {
    if (wish.speed <= 0.0f) {
        return;
    }
    const float current = glm::dot(velocity, wish.dir);
    const float add = std::min(wish.speed, addCap) - current;
    if (add <= 0.0f) {
        return;
    }
    const float gain = std::min(accel * wish.speed * dt, add);
    velocity += wish.dir * gain;
}

// Source-style friction: the stop speed makes the deceleration strong at low speed.
void applyFriction(glm::vec3& velocity, float friction, float stopSpeed, float dt) {
    const float speed = horizontalSpeed(velocity);
    if (speed < kStopEpsilon) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        return;
    }
    const float control = std::max(speed, stopSpeed);
    const float newSpeed = std::max(speed - control * friction * dt, 0.0f);
    velocity.x *= newSpeed / speed;
    velocity.z *= newSpeed / speed;
}

// A slide starts from a sprint: crouch pressed on the ground, fast enough.
void tryStartSlide(Player_t& player, const glm::vec3& velocity, const UserCmd_t& cmd, const MovementSettings_t& settings) {
    const bool crouchPressed = hasButton(cmd.buttonsPressed, EButton::kCrouch);
    if (settings.slide && crouchPressed && horizontalSpeed(velocity) >= settings.slideMinSpeed) {
        player.slideTime = settings.slideDuration;
    }
}

// On the ground: a slide only rubs off speed (no steering); otherwise friction then acceleration.
void groundMove(Player_t& player, glm::vec3& velocity, const Wish_t& wish, const MovementSettings_t& settings, float dt) {
    if (player.slideTime > 0.0f) {
        applyFriction(velocity, settings.groundFriction * settings.slideFriction, settings.groundStopSpeed, dt);
        player.slideTime -= dt;
        if (horizontalSpeed(velocity) < settings.groundStopSpeed) {
            player.slideTime = 0.0f;
        }
        return;
    }
    applyFriction(velocity, settings.groundFriction, settings.groundStopSpeed, dt);
    accelerate(velocity, wish, settings.groundAccelerate, kNoCap, dt);
}

void airMove(glm::vec3& velocity, const Wish_t& wish, const MovementSettings_t& settings, float dt) {
    accelerate(velocity, wish, settings.airAccelerate, settings.airWishCap, dt);
}

}  // namespace

void movePlayer(World_t& world, entt::entity entity, const UserCmd_t& cmd) {
    auto [transform, velocity, player] = world.registry.get<Transform_t, Velocity_t, Player_t>(entity);
    const MovementSettings_t& settings = world.movement;
    const float dt = world::tickDelta(world);

    // Do not trust the view angles coming from the client: clamp them.
    transform.yaw = cmd.yaw;
    transform.pitch = cmd.pitch;
    clampViewAngles(transform.yaw, transform.pitch);

    const Wish_t wish = wishFrom(cmd, settings);
    glm::vec3& linear = velocity.linear;
    const bool jumping = player.onGround && hasButton(cmd.buttonsPressed, EButton::kJump);
    if (!player.onGround) {
        player.slideTime = 0.0f;  // leaving the ground ends a slide
        airMove(linear, wish, settings, dt);
    } else if (jumping) {
        // Jump: no friction this tick, so landing and jumping again keeps the speed (bunnyhop).
        linear.y = settings.jumpSpeed;
        player.slideTime = 0.0f;
        if (!settings.bunnyhop) {
            clampHorizontal(linear, settings.runSpeed);
        }
    } else {
        tryStartSlide(player, linear, cmd, settings);
        groundMove(player, linear, wish, settings, dt);
    }
    clampHorizontal(linear, settings.maxSpeed);
    linear.y -= settings.gravity * dt;  // every tick: ground contact comes back from physics as onGround

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
