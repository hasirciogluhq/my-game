#pragma once

#include "engine/interface/i_platform.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace game {

// The actions a player can perform. Physical keys are bound here (Bindings_t); game code only knows actions.
enum class EAction : uint8_t {
    kForward,
    kBack,
    kLeft,
    kRight,
    kAttack,
    kJump,
    kCrouch,
    kUse,
    kReload,
    kWalk,
    kFreeCam,
    kConsole,  // developer console
    kScoreboard,  // statistics table, shown while held down
    kCount,
};

inline constexpr std::size_t kActionCount = static_cast<std::size_t>(EAction::kCount);

// Action → key name (SDL name: "W", "Space", "`", "Mouse1"...).
using Bindings_t = std::array<std::string, kActionCount>;

namespace input {

// The action's config/command name ("forward", "attack", "console"...).
[[nodiscard]] std::string_view actionName(EAction action);
[[nodiscard]] std::optional<EAction> actionFromName(std::string_view name);

// Default keys. Console: the key under Esc ("`").
[[nodiscard]] Bindings_t defaultBindings();

// Binds an action to a key. If the key name is wrong, that action never fires (the platform ignores unknown names).
void bind(Bindings_t& bindings, EAction action, std::string key);

// File format: each line is "bind <action> <key>" (a key name may contain spaces: "Left Shift").
// If the file does not exist, the defaults stay.
void load(Bindings_t& bindings, const std::filesystem::path& path);
void save(const Bindings_t& bindings, const std::filesystem::path& path);
// Save/load location: assets/config/input.cfg (if assets is not found, config/input.cfg in the working directory).
[[nodiscard]] std::filesystem::path configPath();

[[nodiscard]] bool isDown(const IPlatform& platform, const Bindings_t& bindings, EAction action);
[[nodiscard]] bool wasPressed(const IPlatform& platform, const Bindings_t& bindings, EAction action);

}  // namespace input
}  // namespace game
