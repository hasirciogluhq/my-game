#pragma once

#include "engine/engine/core/types.hpp"

#include <expected>
#include <string>

namespace game::model_loader {

// Assimp ile model dosyasını (obj, gltf, fbx...) tek mesh'e birleştirir.
[[nodiscard]] std::expected<MeshData_t, std::string> load(const std::string& path);

}  // namespace game::model_loader
