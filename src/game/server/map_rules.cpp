#include "game/server/map_rules.hpp"

#include "game/server/test_scene.hpp"

namespace game::maprules {
namespace {

// Moon: the configured move.gravity is multiplied by this (default 20 -> 3 m/s^2).
constexpr float kMoonGravityScale = 0.15f;

}  // namespace

void setup(World_t& world, bool testScene) {
    if (testScene) {
        testscene::spawnRandomProps(world);
    }
    switch (world.map) {
        case EMap::kArena:
            break;
        case EMap::kMoon:
            world.movement.gravity *= kMoonGravityScale;
            break;
    }
}

}  // namespace game::maprules
