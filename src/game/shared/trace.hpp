#pragma once

#include "game/shared/world.hpp"

#include <glm/vec3.hpp>

namespace game::trace {

// Ray trace hit test: world (IPhysics::raycast) + player capsules; the nearest hit wins.
// A player behind a wall is not hit. Server projectiles use it; later it is shared for hitscan / aiming.
enum class ETraceKind { kNone, kWorld, kPlayer };

struct TraceHit_t {
    ETraceKind kind = ETraceKind::kNone;
    float distance = 0.0f;
    glm::vec3 point{0.0f};
    entt::entity entity = entt::null;  // kPlayer: the player that was hit
};

struct TraceParams_t {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};  // normalized
    float maxDistance = 0.0f;
    entt::entity ignore = entt::null;  // usually the shooter
};

[[nodiscard]] TraceHit_t line(const World_t& world, const TraceParams_t& params);

}  // namespace game::trace
