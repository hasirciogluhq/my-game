#pragma once

#include "engine/engine/api.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace game {

// Resolves an assets-relative path such as "models/spot.glb": walks up from the working directory looking for "assets/".
[[nodiscard]] ENGINE_API std::optional<std::filesystem::path> findAssetPath(const std::string& relative);

}  // namespace game
