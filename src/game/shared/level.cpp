#include "game/shared/level.hpp"

#include <array>

namespace game::level {
namespace {

constexpr std::array kBoxes{
    Box_t{{3.5f, 0.5f, -7.0f}, {0.8f, 0.5f, 0.8f}, {0.55f, 0.55f, 0.6f, 1.0f}},   // kaide
    Box_t{{-8.0f, 0.5f, -2.0f}, {0.5f, 0.5f, 0.5f}, {0.8f, 0.2f, 0.2f, 1.0f}},
    Box_t{{-8.0f, 1.5f, -2.0f}, {0.5f, 0.5f, 0.5f}, {0.2f, 0.7f, 0.3f, 1.0f}},
    Box_t{{-6.8f, 0.5f, -2.3f}, {0.5f, 0.5f, 0.5f}, {0.25f, 0.4f, 0.9f, 1.0f}},
    Box_t{{0.0f, 1.5f, -12.0f}, {5.0f, 1.5f, 0.3f}, {0.7f, 0.7f, 0.65f, 1.0f}},   // duvar
};

}  // namespace

std::span<const Box_t> boxes() {
    return kBoxes;
}

void buildPhysics(IPhysics& physics) {
    // Zemin: üst yüzü y=0.
    physics.addStaticBox({0.0f, -0.5f, 0.0f}, {kGroundHalfExtent, 0.5f, kGroundHalfExtent});
    for (const Box_t& box : kBoxes) {
        physics.addStaticBox(box.center, box.halfExtents);
    }
}

}  // namespace game::level
