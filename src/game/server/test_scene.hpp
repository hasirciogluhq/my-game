#pragma once

#include "game/shared/world.hpp"

namespace game::testscene {

// Test content for test_game only (never on the dedicated server): animated models at random places and yaw.
// Called from the map rules (game/server/map_rules.cpp) when LaunchOptions_t::testScene is set.
void spawnRandomProps(World_t& world);

}  // namespace game::testscene
