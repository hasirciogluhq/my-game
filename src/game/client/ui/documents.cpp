#include "game/client/ui/documents.hpp"

#include "engine/interface/i_log.hpp"

namespace game::ui {

bool loadDocuments(IUI &ui) {
  if (!ui.rmlAvailable()) {
    return false;
  }
  if (!ui.loadFont("fonts/DejaVuSans.ttf", true)) {
    GAME_LOG_WARN("client", "arayüz fontu yüklenemedi (fonts/DejaVuSans.ttf)");
  }
  return ui.loadDocument(kHudDocument) &&
         ui.loadDocument(kScoreboardDocument) &&
         ui.loadDocument(kPauseMenuDocument);
}

} // namespace game::ui
