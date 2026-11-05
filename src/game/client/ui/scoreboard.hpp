#pragma once

#include "game/shared/world.hpp"

namespace game::ui {

// Tab istatistik tablosu: dünyadaki tüm karakterler (netId, kim sürüyor, can). visible false ise çizmez.
void drawScoreboard(const World_t& world, bool visible);

}  // namespace game::ui
