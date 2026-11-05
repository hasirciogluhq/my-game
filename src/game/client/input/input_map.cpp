#include "game/client/input/input_map.hpp"

#include "engine/engine/core/asset_path.hpp"

#include <fstream>
#include <sstream>

namespace game::input {
namespace {

struct ActionInfo_t {
    EAction action;
    std::string_view name;
    std::string_view defaultKey;
};

// Yeni aksiyon için tek yer: enum + bu tabloya bir satır.
constexpr std::array<ActionInfo_t, kActionCount> kActions{{
    {EAction::kForward, "forward", "W"},
    {EAction::kBack, "back", "S"},
    {EAction::kLeft, "left", "A"},
    {EAction::kRight, "right", "D"},
    {EAction::kAttack, "attack", "Mouse1"},
    {EAction::kJump, "jump", "Space"},
    {EAction::kCrouch, "crouch", "C"},
    {EAction::kUse, "use", "E"},
    {EAction::kReload, "reload", "R"},
    {EAction::kWalk, "walk", "Left Shift"},
    {EAction::kFreeCam, "freecam", "F"},
    {EAction::kConsole, "console", "`"},
    {EAction::kScoreboard, "scoreboard", "Tab"},
}};

const std::string& keyOf(const Bindings_t& bindings, EAction action) {
    return bindings[static_cast<std::size_t>(action)];
}

}  // namespace

std::string_view actionName(EAction action) {
    return kActions[static_cast<std::size_t>(action)].name;
}

std::optional<EAction> actionFromName(std::string_view name) {
    for (const ActionInfo_t& info : kActions) {
        if (info.name == name) {
            return info.action;
        }
    }
    return std::nullopt;
}

Bindings_t defaultBindings() {
    Bindings_t bindings;
    for (const ActionInfo_t& info : kActions) {
        bindings[static_cast<std::size_t>(info.action)] = std::string(info.defaultKey);
    }
    return bindings;
}

void bind(Bindings_t& bindings, EAction action, std::string key) {
    bindings[static_cast<std::size_t>(action)] = std::move(key);
}

void load(Bindings_t& bindings, const std::filesystem::path& path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream words(line);
        std::string command;
        std::string actionText;
        if (!(words >> command >> actionText) || command != "bind") {
            continue;
        }
        std::string key;
        std::getline(words >> std::ws, key);
        if (const auto action = actionFromName(actionText); action && !key.empty()) {
            bind(bindings, *action, key);
        }
    }
}

void save(const Bindings_t& bindings, const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream file(path);
    file << "# Tuş atamaları. Konsolda: bind <aksiyon> <tuş>\n";
    for (const ActionInfo_t& info : kActions) {
        file << "bind " << info.name << " " << keyOf(bindings, info.action) << "\n";
    }
}

std::filesystem::path configPath() {
    if (const auto dir = findAssetPath("config")) {
        return *dir / "input.cfg";
    }
    return std::filesystem::path("config") / "input.cfg";
}

bool isDown(const IPlatform& platform, const Bindings_t& bindings, EAction action) {
    return platform.keyDownByName(keyOf(bindings, action).c_str());
}

bool wasPressed(const IPlatform& platform, const Bindings_t& bindings, EAction action) {
    return platform.keyPressedByName(keyOf(bindings, action).c_str());
}

}  // namespace game::input
