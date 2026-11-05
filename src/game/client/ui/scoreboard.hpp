#pragma once

#include "engine/interface/i_ui.hpp"
#include "game/shared/world.hpp"

namespace game::ui {

// Skor tablosu (RmlUi, assets/ui/scoreboard.rml): CS2 tarzı geniş tablo,
// oyuncular ve botlar ayrı bölümlerde. visible false iken belge gizlenir ve
// satırlar üretilmez. self: yerel oyuncunun pawn'ı (satırı vurgulanır).
void updateScoreboard(IUI &ui, const World_t &world, entt::entity self,
                      bool visible);

} // namespace game::ui
