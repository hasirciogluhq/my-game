#pragma once

#include "client/scene/scene.hpp"

namespace game::scene {

// Renderer'a küçük yardımcılar (scene/ içinde ortak).
[[nodiscard]] glm::mat4 scaleAt(glm::vec3 center, glm::vec3 size);
[[nodiscard]] MeshHandle_t uploadMesh(IRenderer &renderer,
                                      const MeshData_t &mesh);
[[nodiscard]] MeshHandle_t uploadSkinnedMesh(IRenderer &renderer,
                                             const anim::SkinnedModel_t &model);
[[nodiscard]] MaterialHandle_t
makeMaterial(IRenderer &renderer, const char *name, EShadingModel shading,
             const glm::vec4 &color, EBlendMode blend = EBlendMode::kOpaque);
[[nodiscard]] MaterialHandle_t
litMaterial(IRenderer &renderer, const char *name, const glm::vec4 &color);

} // namespace game::scene
