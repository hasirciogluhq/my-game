#pragma once

#include "engine/engine/api.hpp"
#include "engine/engine/core/types.hpp"

#include <string>

namespace game {

// Test ve skeleton için üretilmiş mesh'ler. Assimp yüklemesi CAssetSystem üzerinden.
// Küp: merkez origin, kenar 1 (-0.5..0.5). Winding CCW (dışarıdan).
[[nodiscard]] ENGINE_API MeshData_t makeBoxMesh(std::string name);

// Düzlem: y=0, x/z -halfExtent..halfExtent, normal +Y.
[[nodiscard]] ENGINE_API MeshData_t makePlaneMesh(std::string name, float halfExtent);

}  // namespace game
