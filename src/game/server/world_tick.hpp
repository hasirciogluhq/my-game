#pragma once

#include "game/shared/world.hpp"

namespace game::world {

// One server tick: input -> ai -> movement -> weapon -> projectiles -> owners, then the tick increments.
void tick(World_t& world);

}  // namespace game::world
