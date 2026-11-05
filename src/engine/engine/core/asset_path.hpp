#pragma once

#include "engine/engine/api.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace game {

// "models/spot.glb" gibi assets-göreli yolu çözer: çalışma dizininden yukarı doğru "assets/" arar.
[[nodiscard]] ENGINE_API std::optional<std::filesystem::path> findAssetPath(const std::string& relative);

}  // namespace game
