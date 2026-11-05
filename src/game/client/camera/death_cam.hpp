#pragma once

#include "game/shared/world.hpp"

#include <glm/vec3.hpp>

namespace game::death_cam {

// Camera of a player who has no pawn: an aerial view from above the map, slowly orbiting while looking at the characters
// (like the Battlefield / GTA V loading screen).
struct DeathCam_t {
    float angle = 0.0f;  // rotation angle (rad)
};

struct Pose_t {
    glm::vec3 position{0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
};

// Advances the angle.
void update(DeathCam_t& cam, float dt);

// Focus point: the middle of the living characters (the map center if nobody is alive). The camera orbits around it.
[[nodiscard]] Pose_t pose(const DeathCam_t& cam, const World_t& world);

}  // namespace game::death_cam
