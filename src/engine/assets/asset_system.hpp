#pragma once

#include "engine/assets/api.hpp"
#include "engine/interface/i_assets.hpp"

#include <unordered_map>

namespace game {

// A class because it holds a cache. Interface: IAssets (Assets001).
class ASSETS_API CAssetSystem final : public IAssets {
public:
    [[nodiscard]] std::expected<std::shared_ptr<const MeshData_t>, std::string> loadMesh(const std::string& path) override;
    [[nodiscard]] std::expected<std::shared_ptr<const anim::SkinnedModel_t>, std::string> loadSkinned(const std::string& path) override;

private:
    std::unordered_map<std::string, std::shared_ptr<const MeshData_t>> m_meshes;
    std::unordered_map<std::string, std::shared_ptr<const anim::SkinnedModel_t>> m_skinned;
};

}  // namespace game
