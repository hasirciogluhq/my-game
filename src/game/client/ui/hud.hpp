#pragma once

#include "engine/interface/i_ui.hpp"
#include "game/shared/world.hpp"

namespace game::ui {

// Oyun içi HUD (RmlUi, assets/ui/hud.rml): nişangah + yerel oyuncunun canı.
// self: sürülen pawn ya da entt::null (HUD gizlenir).
void updateHud(IUI &ui, const World_t &world, entt::entity self);

} // namespace game::ui
