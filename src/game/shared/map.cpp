#include "game/shared/map.hpp"

#include <array>
#include <utility>

namespace game {
namespace {

constexpr std::array kMapNames{
    std::pair{EMap::kArena, std::string_view{"arena"}},
    std::pair{EMap::kMoon, std::string_view{"moon"}},
};

}  // namespace

std::string_view mapName(EMap map) {
    for (const auto& [id, name] : kMapNames) {
        if (id == map) {
            return name;
        }
    }
    return "unknown";
}

std::optional<EMap> mapFromName(std::string_view name) {
    for (const auto& [id, mapped] : kMapNames) {
        if (mapped == name) {
            return id;
        }
    }
    return std::nullopt;
}

}  // namespace game
