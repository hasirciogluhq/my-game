#pragma once

#include "engine/interface/i_ui.hpp"
#include "game/client/input/input_sampler.hpp"

namespace game::ui {

// Duraklatma menüsünün sonucu: kapanma ya da çıkış isteği. Diğer eylemler
// (hassasiyet) burada uygulanır.
struct PauseResult_t {
  bool resume = false;
  bool quit = false;
};

void showPauseMenu(IUI &ui, bool open);

// Menüden gelen bir eylemi işler (resume, sensitivity <değer>, quit).
// Tanınmayan eylem yok sayılır.
void handlePauseAction(const UiAction_t &action, CInputSampler &input,
                       PauseResult_t &result);

} // namespace game::ui
