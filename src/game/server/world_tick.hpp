#pragma once

#include "game/shared/world.hpp"

namespace game::world {

// Bir server tick'i: input -> ai -> hareket -> silah -> mermiler -> sahiplik, sonra tick artar.
void tick(World_t& world);

}  // namespace game::world
