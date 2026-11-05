#pragma once

#include "engine/engine/anim/skinned_model.hpp"

#include <expected>
#include <string>

namespace game::model_loader {

// Assimp ile iskeletli modeli (gltf, fbx...) yükler: tek mesh + iskelet + tüm animasyon klipleri.
// Vertex başına en çok 4 eklem; eklem sayısı kMaxBones'u aşarsa hata.
[[nodiscard]] std::expected<anim::SkinnedModel_t, std::string> loadSkinned(const std::string& path);

}  // namespace game::model_loader
