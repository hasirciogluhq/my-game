#include "game/server/systems/animation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include <glm/geometric.hpp>

namespace game::systems {
namespace {

struct Nearest_t {
    glm::vec3 offset{0.0f};  // animasyonlu entity'den oyuncuya (yatay)
    float distance = std::numeric_limits<float>::max();
};

// Entity'ye en yakın canlı oyuncu pawn'ı (bot'lar da Player_t taşır).
Nearest_t nearestPlayer(const World_t& world, const glm::vec3& from) {
    Nearest_t nearest;
    for (const auto [entity, player, transform] : world.registry.view<const Player_t, const Transform_t>().each()) {
        const glm::vec3 offset{transform.position.x - from.x, 0.0f, transform.position.z - from.z};
        if (const float distance = glm::length(offset); distance < nearest.distance) {
            nearest = {.offset = offset, .distance = distance};
        }
    }
    return nearest;
}

// yaw=0 -Z'ye bakar: ileri = (-sin yaw, -cos yaw).
float yawToward(const glm::vec3& offset) {
    return std::atan2(-offset.x, -offset.z);
}

// from'dan to'ya en kısa yoldan, en çok maxStep kadar döner.
float turnToward(float from, float to, float maxStep) {
    float delta = std::fmod(to - from, 2.0f * std::numbers::pi_v<float>);
    if (delta > std::numbers::pi_v<float>) {
        delta -= 2.0f * std::numbers::pi_v<float>;
    } else if (delta < -std::numbers::pi_v<float>) {
        delta += 2.0f * std::numbers::pi_v<float>;
    }
    return from + std::clamp(delta, -maxStep, maxStep);
}

EAnimState chooseState(EAnimState current, float distance) {
    const float limit = current == EAnimState::kWave ? kGreetExitRadius : kGreetEnterRadius;
    return distance < limit ? EAnimState::kWave : EAnimState::kIdle;
}

void setState(Animator_t& animator, EAnimState state, uint32_t tick) {
    if (animator.state == state) {
        return;
    }
    animator.prevState = animator.state;
    animator.prevStartTick = animator.startTick;
    animator.state = state;
    animator.startTick = tick;
}

}  // namespace

void animation(World_t& world) {
    const float dt = world::tickDelta(world);
    for (const auto [entity, animator, transform] : world.registry.view<Animator_t, Transform_t>().each()) {
        const Nearest_t nearest = nearestPlayer(world, transform.position);
        setState(animator, chooseState(animator.state, nearest.distance), world.tick);
        if (nearest.distance < kLookRadius && nearest.distance > 0.01f) {
            transform.yaw = turnToward(transform.yaw, yawToward(nearest.offset), kTurnSpeed * dt);
        }
    }
}

}  // namespace game::systems
