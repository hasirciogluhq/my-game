#pragma once

#include "engine/interface/i_physics.hpp"
#include "game/shared/map.hpp"

#include <span>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace game::level {

// Static world boxes. The server uses them for physics; the client draws the same list.
struct Box_t {
    glm::vec3 center;
    glm::vec3 halfExtents;
    glm::vec4 color;
};

inline constexpr float kGroundHalfExtent = 50.0f;

[[nodiscard]] std::span<const Box_t> boxes(EMap map);

// Adds the ground and the map's boxes to the physics world.
void buildPhysics(IPhysics& physics, EMap map);

}  // namespace game::level
