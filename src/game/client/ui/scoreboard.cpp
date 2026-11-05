#include "game/client/ui/scoreboard.hpp"

#include "game/shared/components.hpp"

#include <imgui.h>

namespace game::ui {

void drawScoreboard(const World_t& world, bool visible) {
    if (!visible) {
        return;
    }
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({size.x * 0.5f, size.y * 0.3f}, ImGuiCond_Always, {0.5f, 0.0f});
    ImGui::SetNextWindowBgAlpha(0.75f);
    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("##scoreboard", nullptr, kFlags)) {
        if (ImGui::BeginTable("players", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("netId");
            ImGui::TableSetupColumn("oyuncu");
            ImGui::TableSetupColumn("can");
            ImGui::TableHeadersRow();
            for (const auto [entity, replicated, player] : world.registry.view<const Replicated_t, const Player_t>().each()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%u", replicated.netId);
                ImGui::TableNextColumn();
                if (player.playerId == 0) {
                    ImGui::TextUnformatted("bot");
                } else {
                    ImGui::Text("oyuncu %u", player.playerId);
                }
                ImGui::TableNextColumn();
                ImGui::Text("%u", player.health);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

}  // namespace game::ui
