#pragma once

#include "engine/interface/i_ui.hpp"

namespace game::ui {

// RmlUi belgeleri (assets-göreli). Hepsi assets/ui/ altında.
inline constexpr char kHudDocument[] = "ui/hud.rml";
inline constexpr char kScoreboardDocument[] = "ui/scoreboard.rml";
inline constexpr char kPauseMenuDocument[] = "ui/pause_menu.rml";

// Fontu ve belgeleri yükler (hepsi gizli başlar). RmlUi yoksa false.
[[nodiscard]] bool loadDocuments(IUI &ui);

} // namespace game::ui
