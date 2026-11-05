#pragma once

#include "engine/assets/api.hpp"
#include "engine/interface/i_assets.hpp"

#include <unordered_map>

namespace game {

// Önbellek tuttuğu için sınıf. Arayüz: IAssets (Assets001).
class ASSETS_API CAssetSystem final : public IAssets {
public:
    [[nodiscard]] std::expected<std::shared_ptr<const MeshData_t>, std::string> loadMesh(const std::string& path) override;

private:
    std::unordered_map<std::string, std::shared_ptr<const MeshData_t>> m_meshes;
};

}  // namespace game
