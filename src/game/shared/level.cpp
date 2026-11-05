#include "game/shared/level.hpp"

#include <array>

namespace game::level {
namespace {

constexpr std::array kArenaBoxes{
    Box_t{{3.5f, 0.5f, -7.0f}, {0.8f, 0.5f, 0.8f}, {0.55f, 0.55f, 0.6f, 1.0f}},   // pedestal
    Box_t{{-8.0f, 0.5f, -2.0f}, {0.5f, 0.5f, 0.5f}, {0.8f, 0.2f, 0.2f, 1.0f}},
    Box_t{{-8.0f, 1.5f, -2.0f}, {0.5f, 0.5f, 0.5f}, {0.2f, 0.7f, 0.3f, 1.0f}},
    Box_t{{-6.8f, 0.5f, -2.3f}, {0.5f, 0.5f, 0.5f}, {0.25f, 0.4f, 0.9f, 1.0f}},
    Box_t{{0.0f, 1.5f, -12.0f}, {5.0f, 1.5f, 0.3f}, {0.7f, 0.7f, 0.65f, 1.0f}},   // wall
};

constexpr std::array kMoonBoxes{
    Box_t{{-6.0f, 2.0f, -6.0f}, {0.5f, 2.0f, 0.5f}, {0.7f, 0.7f, 0.72f, 1.0f}},  // pillars, 4 m tall
    Box_t{{6.0f, 2.0f, -6.0f}, {0.5f, 2.0f, 0.5f}, {0.7f, 0.7f, 0.72f, 1.0f}},
    Box_t{{-6.0f, 2.0f, 6.0f}, {0.5f, 2.0f, 0.5f}, {0.7f, 0.7f, 0.72f, 1.0f}},
    Box_t{{6.0f, 2.0f, 6.0f}, {0.5f, 2.0f, 0.5f}, {0.7f, 0.7f, 0.72f, 1.0f}},
    Box_t{{0.0f, 0.5f, 0.0f}, {2.0f, 0.5f, 2.0f}, {0.4f, 0.45f, 0.5f, 1.0f}},    // low platform in the middle
};

}  // namespace

std::span<const Box_t> boxes(EMap map) {
    switch (map) {
        case EMap::kMoon:
            return kMoonBoxes;
        case EMap::kArena:
            break;
    }
    return kArenaBoxes;
}

void buildPhysics(IPhysics& physics, EMap map) {
    // Ground: top surface at y=0.
    physics.addStaticBox({0.0f, -0.5f, 0.0f}, {kGroundHalfExtent, 0.5f, kGroundHalfExtent});
    for (const Box_t& box : boxes(map)) {
        physics.addStaticBox(box.center, box.halfExtents);
    }
}

}  // namespace game::level
