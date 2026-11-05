#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Bot controller'ları kendi komutunu üretir (devriye: ileri yürü, yavaşça dön).
void ai(World_t& world);

}  // namespace game::systems
