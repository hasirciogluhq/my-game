#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Saldırı basılıysa (cooldown geçtiyse) pawn'ın gözünden mermi üretir.
void weapon(World_t& world);

}  // namespace game::systems
