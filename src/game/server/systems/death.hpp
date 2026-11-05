#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Characters at 0 health are deleted (despawned). When a bot dies, its AI controller goes too. When a player dies, its controller
// (connection) stays but has no pawn; the client sees "no pawn" and switches to the death camera.
// The view target of controllers watching the dead pawn is cleared.
void death(World_t& world);

}  // namespace game::systems
