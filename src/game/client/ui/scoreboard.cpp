#include "game/client/ui/scoreboard.hpp"

#include <algorithm>
#include <format>
#include <string>
#include <vector>

#include "game/client/ui/documents.hpp"
#include "game/shared/components.hpp"

namespace game::ui {

namespace {

constexpr uint32_t kMaxHealth = 100;
constexpr uint32_t kLowHealth = 30;

struct Row_t {
  uint32_t netId = 0;
  uint32_t playerId = 0; // 0 = bot
  uint32_t health = 0;
  bool self = false;
};

std::vector<Row_t> collectRows(const World_t &world, entt::entity self) {
  std::vector<Row_t> rows;
  for (const auto [entity, replicated, player] :
       world.registry.view<const Replicated_t, const Player_t>().each()) {
    rows.push_back({.netId = replicated.netId,
                    .playerId = player.playerId,
                    .health = player.health,
                    .self = entity == self});
  }
  // Higher health on top; on a tie, the smaller netId.
  std::ranges::sort(rows, [](const Row_t &a, const Row_t &b) {
    return a.health != b.health ? a.health > b.health : a.netId < b.netId;
  });
  return rows;
}

std::string rowRml(const Row_t &row, size_t index) {
  const bool bot = row.playerId == 0;
  const uint32_t health = std::min(row.health, kMaxHealth);
  const std::string name = bot ? std::format("Bot {}", row.netId)
                               : std::format("Player {}", row.playerId);
  const std::string initial = bot ? "B" : std::to_string(row.playerId % 10);
  const char *classes = row.self         ? "row self"
                        : index % 2 == 1 ? "row alt"
                                         : "row";
  return std::format(
      R"(<div class="{}{}"><div class="col rank">{}</div>)"
      R"(<div class="col name"><div class="avatar">{}</div>{}{}</div>)"
      R"(<div class="col health"><div class="hp-num">{}</div><div class="hp-track"><div class="hp-fill" style="width: {}%;"></div></div></div>)"
      R"(<div class="col id">{}</div></div>)",
      classes, health <= kLowHealth ? " low" : "", index + 1, initial, name,
      row.self ? R"(<div class="tag">YOU</div>)" : "", health,
      health * 100 / kMaxHealth, row.netId);
}

std::string sectionRml(const std::vector<Row_t> &rows, bool bots) {
  std::string html;
  size_t index = 0;
  for (const Row_t &row : rows) {
    if ((row.playerId == 0) == bots) {
      html += rowRml(row, index++);
    }
  }
  return html;
}

} // namespace

void updateScoreboard(IUI &ui, const World_t &world, entt::entity self,
                      bool visible) {
  if (!visible) {
    ui.hideDocument(kScoreboardDocument);
    return;
  }
  if (!ui.isDocumentVisible(kScoreboardDocument)) {
    ui.showDocument(kScoreboardDocument, false);
  }
  const std::vector<Row_t> rows = collectRows(world, self);
  const auto botCount = static_cast<size_t>(std::ranges::count_if(
      rows, [](const Row_t &row) { return row.playerId == 0; }));
  const size_t playerCount = rows.size() - botCount;

  ui.setElementRml(kScoreboardDocument, "summary",
                   std::format("{} characters", rows.size()).c_str());
  ui.setElementRml(kScoreboardDocument, "players-count",
                   std::format("{}", playerCount).c_str());
  ui.setElementRml(kScoreboardDocument, "bots-count",
                   std::format("{}", botCount).c_str());
  ui.setElementRml(kScoreboardDocument, "players-rows",
                   sectionRml(rows, false).c_str());
  ui.setElementRml(kScoreboardDocument, "bots-rows",
                   sectionRml(rows, true).c_str());
}

} // namespace game::ui
