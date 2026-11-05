#include "engine/assets/asset_system.hpp"

#include "engine/assets/model_loader.hpp"
#include "engine/assets/skinned_loader.hpp"
#include "engine/engine/core/asset_path.hpp"

namespace game {

std::expected<std::shared_ptr<const MeshData_t>, std::string> CAssetSystem::loadMesh(const std::string& path) {
    if (const auto it = m_meshes.find(path); it != m_meshes.end()) {
        return it->second;
    }

    const auto file = findAssetPath(path);
    if (!file) {
        return std::unexpected("asset not found: " + path);
    }
    auto loaded = model_loader::load(file->string());
    if (!loaded) {
        return std::unexpected(loaded.error());
    }

    auto mesh = std::make_shared<const MeshData_t>(std::move(*loaded));
    m_meshes.emplace(path, mesh);
    return mesh;
}

std::expected<std::shared_ptr<const anim::SkinnedModel_t>, std::string> CAssetSystem::loadSkinned(const std::string& path) {
    if (const auto it = m_skinned.find(path); it != m_skinned.end()) {
        return it->second;
    }

    const auto file = findAssetPath(path);
    if (!file) {
        return std::unexpected("asset not found: " + path);
    }
    auto loaded = model_loader::loadSkinned(file->string());
    if (!loaded) {
        return std::unexpected(loaded.error());
    }

    auto model = std::make_shared<const anim::SkinnedModel_t>(std::move(*loaded));
    m_skinned.emplace(path, model);
    return model;
}

}  // namespace game
