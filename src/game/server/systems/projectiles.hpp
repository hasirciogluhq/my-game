#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Deletes the projectiles whose lifetime ended or that hit something; deals damage on a player hit.
void projectiles(World_t& world);

}  // namespace game::systems
