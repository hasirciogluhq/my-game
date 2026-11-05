#pragma once

#include "engine/interface/i_ui.hpp"
#include "game/shared/world.hpp"

namespace game::ui {

// Scoreboard (RmlUi, assets/ui/scoreboard.rml): a wide CS2-style table,
// players and bots in separate sections. When visible is false, the document is hidden and
// no rows are produced. self: the local player's pawn (its row is highlighted).
void updateScoreboard(IUI &ui, const World_t &world, entt::entity self,
                      bool visible);

} // namespace game::ui
