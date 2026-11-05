#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Sets CommandQueue_t::current to the next command from each controller's queue; writes the ack to the pawn.
void input(World_t& world);

}  // namespace game::systems
