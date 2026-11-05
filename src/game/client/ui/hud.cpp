#include "game/client/ui/hud.hpp"

#include <algorithm>
#include <string>

#include "game/client/ui/documents.hpp"
#include "game/shared/components.hpp"

namespace game::ui {

namespace {

constexpr uint32_t kMaxHealth = 100;
constexpr uint32_t kLowHealth = 30;

} // namespace

void updateHud(IUI &ui, const World_t &world, entt::entity self) {
  if (self == entt::null) {
    ui.hideDocument(kHudDocument);
    return;
  }
  if (!ui.isDocumentVisible(kHudDocument)) {
    ui.showDocument(kHudDocument, false);
  }
  const uint32_t health =
      std::min(world.registry.get<Player_t>(self).health, kMaxHealth);
  ui.setElementRml(kHudDocument, "health-value",
                   std::to_string(health).c_str());
  ui.setElementStyle(kHudDocument, "health-fill", "width",
                     (std::to_string(health * 100 / kMaxHealth) + "%").c_str());
  ui.setElementClass(kHudDocument, "health-panel", "low", health <= kLowHealth);
}

} // namespace game::ui
