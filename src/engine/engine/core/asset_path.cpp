#include "engine/engine/core/asset_path.hpp"

namespace game {

std::optional<std::filesystem::path> findAssetPath(const std::string& relative) {
    std::error_code error;
    std::filesystem::path dir = std::filesystem::current_path(error);
    while (!error && !dir.empty()) {
        const std::filesystem::path candidate = dir / "assets" / relative;
        if (std::filesystem::exists(candidate, error)) {
            return candidate;
        }
        if (dir == dir.parent_path()) {
            break;
        }
        dir = dir.parent_path();
    }
    return std::nullopt;
}

}  // namespace game
