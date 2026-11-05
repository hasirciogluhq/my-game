#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Her pawn'a controller'ının komutunu uygular (controller yoksa durur, yerçekimi sürer).
void movement(World_t& world);

}  // namespace game::systems
