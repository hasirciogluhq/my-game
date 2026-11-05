#pragma once

#include "engine/interface/i_ui.hpp"
#include "game/client/input/input_sampler.hpp"

namespace game::ui {

// Result of the pause menu: a close or quit request. Other actions
// (sensitivity) are applied here.
struct PauseResult_t {
  bool resume = false;
  bool quit = false;
};

void showPauseMenu(IUI &ui, bool open);

// Handles an action coming from the menu (resume, sensitivity <value>, quit).
// Unrecognized actions are ignored.
void handlePauseAction(const UiAction_t &action, CInputSampler &input,
                       PauseResult_t &result);

} // namespace game::ui
