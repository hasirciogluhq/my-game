#pragma once

#include "engine/engine/anim/skinned_model.hpp"

#include <expected>
#include <string>

namespace game::model_loader {

// Loads a skinned model (gltf, fbx...) using Assimp: a single mesh + skeleton + all animation clips.
// At most 4 joints per vertex; an error if the joint count exceeds kMaxBones.
[[nodiscard]] std::expected<anim::SkinnedModel_t, std::string> loadSkinned(const std::string& path);

}  // namespace game::model_loader
