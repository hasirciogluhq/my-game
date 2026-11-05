#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace game {

// A map is one world: its geometry (level::boxes), its rules (game/server/map_rules) and its name in server.cfg (map.name).
enum class EMap : uint32_t {
    kArena = 0,  // the default test arena
    kMoon = 1,   // same rules, low gravity, pillars
};

[[nodiscard]] std::string_view mapName(EMap map);
[[nodiscard]] std::optional<EMap> mapFromName(std::string_view name);

}  // namespace game
