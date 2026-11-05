#pragma once

#include "engine/engine/anim/skinned_model.hpp"
#include "engine/engine/core/types.hpp"

#include <expected>
#include <memory>
#include <string>

namespace game {

// assets.dll interface. Paths are relative to the assets/ folder: "models/spot.glb".
// The same path is loaded once; after that the shared pointer is returned.
class IAssets {
public:
    virtual ~IAssets() = default;
    [[nodiscard]] virtual std::expected<std::shared_ptr<const MeshData_t>, std::string> loadMesh(const std::string& path) = 0;
    // Skinned model (gltf, fbx...): mesh + skeleton + animation clips.
    [[nodiscard]] virtual std::expected<std::shared_ptr<const anim::SkinnedModel_t>, std::string> loadSkinned(const std::string& path) = 0;
};

inline constexpr char kAssetsInterface[] = "Assets001";

}  // namespace game
