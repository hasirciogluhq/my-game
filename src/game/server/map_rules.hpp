#pragma once

#include "game/shared/world.hpp"

namespace game::maprules {

// Runs once when the server opens the map, before any player connects. Each map has its own rules here:
// content (test props) and changes to world.movement (sent to clients in WELCOME, so prediction follows them).
void setup(World_t& world, bool testScene);

}  // namespace game::maprules
