#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Ömrü bitenleri / bir şeye çarpanları siler, oyuncu isabetinde hasar verir.
void projectiles(World_t& world);

}  // namespace game::systems
