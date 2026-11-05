#pragma once

#include "game/shared/world.hpp"

namespace game::testscene {

// Test content for test_server only (never on the dedicated server): animated models at random places and yaw.
void spawnRandomProps(World_t& world);

}  // namespace game::testscene
