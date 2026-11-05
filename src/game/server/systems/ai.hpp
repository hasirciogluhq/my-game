#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Bot controllers produce their own command (patrol: walk forward, turn slowly).
void ai(World_t& world);

}  // namespace game::systems
