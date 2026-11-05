#pragma once

#include "game/shared/world.hpp"

#include <glm/vec3.hpp>

namespace game::trace {

// Ray trace hit test: dünya (IPhysics::raycast) + oyuncu kapsülleri, en yakın isabet kazanır.
// Duvarın arkasındaki oyuncu vurulmaz. Server mermileri, ileride hitscan / nişan için ortak kullanılır.
enum class ETraceKind { kNone, kWorld, kPlayer };

struct TraceHit_t {
    ETraceKind kind = ETraceKind::kNone;
    float distance = 0.0f;
    glm::vec3 point{0.0f};
    entt::entity entity = entt::null;  // kPlayer ise vurulan oyuncu
};

struct TraceParams_t {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};  // normalize
    float maxDistance = 0.0f;
    entt::entity ignore = entt::null;  // genelde ateş eden oyuncu
};

[[nodiscard]] TraceHit_t line(const World_t& world, const TraceParams_t& params);

}  // namespace game::trace
