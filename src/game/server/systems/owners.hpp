#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Sets Player_t::playerId from the controller that drives the pawn (the replicated "who is driving this").
void owners(World_t& world);

}  // namespace game::systems
