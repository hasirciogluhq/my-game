#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// If attack is pressed (cooldown has passed), creates a projectile from the pawn's eye.
void weapon(World_t& world);

}  // namespace game::systems
