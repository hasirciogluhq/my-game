#include "game/client/camera/death_cam.hpp"

#include "game/shared/components.hpp"

#include <cmath>

#include <glm/geometric.hpp>

namespace game::death_cam {
namespace {

constexpr float kTurnSpeed = 0.12f;  // rad/s: slow turn
constexpr float kRadius = 16.0f;
constexpr float kHeight = 12.0f;

glm::vec3 focusPoint(const World_t& world) {
    glm::vec3 sum{0.0f};
    int count = 0;
    for (const auto [entity, player, transform] : world.registry.view<const Player_t, const Transform_t>().each()) {
        sum += transform.position;
        ++count;
    }
    return count > 0 ? sum / static_cast<float>(count) : glm::vec3{0.0f};
}

}  // namespace

void update(DeathCam_t& cam, float dt) {
    cam.angle += kTurnSpeed * dt;
}

Pose_t pose(const DeathCam_t& cam, const World_t& world) {
    const glm::vec3 focus = focusPoint(world);
    const glm::vec3 position = focus + glm::vec3{std::sin(cam.angle) * kRadius, kHeight, std::cos(cam.angle) * kRadius};
    const glm::vec3 toFocus = glm::normalize(focus - position);
    // Camera direction: (cos p * sin y, sin p, -cos p * cos y).
    return {.position = position, .yaw = std::atan2(toFocus.x, -toFocus.z), .pitch = std::asin(toFocus.y)};
}

}  // namespace game::death_cam
