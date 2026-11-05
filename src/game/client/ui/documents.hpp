#pragma once

#include "engine/interface/i_ui.hpp"

namespace game::ui {

// RmlUi documents (assets-relative). All under assets/ui/.
inline constexpr char kHudDocument[] = "ui/hud.rml";
inline constexpr char kScoreboardDocument[] = "ui/scoreboard.rml";
inline constexpr char kPauseMenuDocument[] = "ui/pause_menu.rml";

// Loads the font and the documents (all start hidden). False if RmlUi is not available.
[[nodiscard]] bool loadDocuments(IUI &ui);

} // namespace game::ui
