#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Applies each pawn's controller's command to it (if there is no controller, it stops; gravity keeps going).
void movement(World_t& world);

}  // namespace game::systems
