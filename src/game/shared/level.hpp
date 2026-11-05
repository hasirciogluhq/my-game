#pragma once

#include "engine/interface/i_physics.hpp"

#include <span>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace game::level {

// Statik dünya kutuları. Server fizik için, client çizim için aynı listeyi kullanır.
struct Box_t {
    glm::vec3 center;
    glm::vec3 halfExtents;
    glm::vec4 color;
};

[[nodiscard]] std::span<const Box_t> boxes();

inline constexpr float kGroundHalfExtent = 50.0f;

// Zemin + kutuları fizik dünyasına ekler.
void buildPhysics(IPhysics& physics);

}  // namespace game::level
