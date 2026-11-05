#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Player_t::playerId'yi pawn'ı süren controller'dan eşitler (replicate edilen "bunu kim sürüyor").
void owners(World_t& world);

}  // namespace game::systems
