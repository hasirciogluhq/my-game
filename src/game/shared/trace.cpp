#include "game/shared/trace.hpp"

#include "engine/engine/core/ray.hpp"
#include "engine/interface/i_physics.hpp"

namespace game::trace {

TraceHit_t line(const World_t& world, const TraceParams_t& params) {
    TraceHit_t best;
    float limit = params.maxDistance;

    if (world.physics != nullptr) {
        const RayHit_t hit = world.physics->raycast(params.origin, params.direction, limit);
        if (hit.hit) {
            best = {.kind = ETraceKind::kWorld, .distance = hit.distance, .point = hit.point};
            limit = hit.distance;
        }
    }

    // Oyuncular fizikte ışına görünmez (CharacterVirtual): kapsüllere analitik bakılır.
    for (const auto [entity, transform, player] : world.registry.view<const Transform_t, const Player_t>().each()) {
        if (entity == params.ignore) {
            continue;
        }
        const auto distance = rayCapsule(params.origin, params.direction, limit, transform.position, kCharacterRadius, kCharacterHeight);
        if (distance) {
            best = {.kind = ETraceKind::kPlayer, .distance = *distance, .point = params.origin + params.direction * *distance, .entity = entity};
            limit = *distance;
        }
    }
    return best;
}

}  // namespace game::trace
