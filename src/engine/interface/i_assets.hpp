#pragma once

#include "engine/engine/anim/skinned_model.hpp"
#include "engine/engine/core/types.hpp"

#include <expected>
#include <memory>
#include <string>

namespace game {

// assets.dll arayüzü. Yol assets/ klasörüne göredir: "models/spot.glb".
// Aynı yol bir kez yüklenir, sonra paylaşılan pointer döner.
class IAssets {
public:
    virtual ~IAssets() = default;
    [[nodiscard]] virtual std::expected<std::shared_ptr<const MeshData_t>, std::string> loadMesh(const std::string& path) = 0;
    // İskeletli model (gltf, fbx...): mesh + iskelet + animasyon klipleri.
    [[nodiscard]] virtual std::expected<std::shared_ptr<const anim::SkinnedModel_t>, std::string> loadSkinned(const std::string& path) = 0;
};

inline constexpr char kAssetsInterface[] = "Assets001";

}  // namespace game
