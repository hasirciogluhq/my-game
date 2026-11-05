#pragma once

#include "game/shared/world.hpp"

namespace game::ui {

// Oyun içi HUD: nişangah + yerel oyuncunun canı. ImGui frame'i açıkken çağrılır. self: sürülen pawn ya da entt::null.
void drawHud(const World_t& world, entt::entity self);

}  // namespace game::ui
