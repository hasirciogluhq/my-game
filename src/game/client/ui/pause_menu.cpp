#include "game/client/ui/pause_menu.hpp"

#include <charconv>

#include "game/client/ui/documents.hpp"

namespace game::ui {

namespace {

// Kaydırıcı 1..100; her adım 0.0001 rad/piksel (varsayılan 0.0025 = 25).
constexpr float kSensitivityPerStep = 0.0001f;

} // namespace

void showPauseMenu(IUI &ui, bool open) {
  if (open) {
    ui.showDocument(kPauseMenuDocument, true);
  } else {
    ui.hideDocument(kPauseMenuDocument);
  }
}

void handlePauseAction(const UiAction_t &action, CInputSampler &input,
                       PauseResult_t &result) {
  if (action.name == "resume") {
    result.resume = true;
  } else if (action.name == "quit") {
    result.quit = true;
  } else if (action.name == "sensitivity") {
    float step = 0.0f;
    const auto parsed = std::from_chars(
        action.value.data(), action.value.data() + action.value.size(), step);
    if (parsed.ec == std::errc{} && step > 0.0f) {
      input.setMouseSensitivity(step * kSensitivityPerStep);
    }
  }
}

} // namespace game::ui
