#include "game/client/ui/hud.hpp"

#include <imgui.h>

namespace game::ui {

namespace {

constexpr ImU32 kCrosshairColor = IM_COL32(255, 255, 255, 220);
constexpr float kCrosshairSize = 8.0f;

void drawCrosshair(ImDrawList& list, ImVec2 center) {
    list.AddLine({center.x - kCrosshairSize, center.y}, {center.x + kCrosshairSize, center.y}, kCrosshairColor, 2.0f);
    list.AddLine({center.x, center.y - kCrosshairSize}, {center.x, center.y + kCrosshairSize}, kCrosshairColor, 2.0f);
}

}  // namespace

void drawHud(const World_t& world, entt::entity self) {
    if (self == entt::null) {
        return;
    }
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    ImDrawList& list = *ImGui::GetBackgroundDrawList();
    drawCrosshair(list, {size.x * 0.5f, size.y * 0.5f});

    const Player_t& player = world.registry.get<Player_t>(self);
    char text[32];
    std::snprintf(text, sizeof(text), "+ %u", player.health);
    list.AddText(ImGui::GetFont(), 32.0f, {24.0f, size.y - 56.0f}, IM_COL32(255, 90, 90, 255), text);
}

}  // namespace game::ui
