#pragma once

#include "engine/interface/i_ui.hpp"
#include "game/shared/world.hpp"

namespace game::ui {

// In-game HUD (RmlUi, assets/ui/hud.rml): crosshair + the local player's health.
// self: the pawn being driven, or entt::null (the HUD is hidden).
void updateHud(IUI &ui, const World_t &world, entt::entity self);

} // namespace game::ui
