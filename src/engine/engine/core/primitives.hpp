#pragma once

#include "engine/engine/api.hpp"
#include "engine/engine/core/types.hpp"

#include <string>

namespace game {

// Meshes generated for tests and the skeleton. Assimp loading goes through CAssetSystem.
// Cube: centered at the origin, edge 1 (-0.5..0.5). Winding CCW (seen from outside).
[[nodiscard]] ENGINE_API MeshData_t makeBoxMesh(std::string name);

// Plane: y=0, x/z -halfExtent..halfExtent, normal +Y.
[[nodiscard]] ENGINE_API MeshData_t makePlaneMesh(std::string name, float halfExtent);

}  // namespace game
