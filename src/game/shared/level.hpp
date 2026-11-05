#pragma once

#include "engine/interface/i_physics.hpp"

#include <span>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace game::level {

// Static world boxes. The server uses them for physics; the client uses the same list for drawing.
struct Box_t {
    glm::vec3 center;
    glm::vec3 halfExtents;
    glm::vec4 color;
};

[[nodiscard]] std::span<const Box_t> boxes();

inline constexpr float kGroundHalfExtent = 50.0f;

// Adds the ground and the boxes to the physics world.
void buildPhysics(IPhysics& physics);

}  // namespace game::level
