#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Her controller'ın kuyruğundan sıradaki komutu CommandQueue_t::current yapar; ack'i pawn'a yazar.
void input(World_t& world);

}  // namespace game::systems
