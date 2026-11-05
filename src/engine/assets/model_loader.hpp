#pragma once

#include "engine/engine/core/types.hpp"

#include <expected>
#include <string>

namespace game::model_loader {

// Merges a model file (obj, gltf, fbx...) into a single mesh using Assimp.
[[nodiscard]] std::expected<MeshData_t, std::string> load(const std::string& path);

}  // namespace game::model_loader
