#include "client/scene/render_helpers.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace game::scene {

glm::mat4 scaleAt(glm::vec3 center, glm::vec3 size) {
  return glm::translate(glm::mat4{1.0f}, center) *
         glm::scale(glm::mat4{1.0f}, size);
}

MeshHandle_t uploadMesh(IRenderer &renderer, const MeshData_t &mesh) {
  return renderer.createMesh(makeMeshDesc<Vertex_t, uint32_t>(
      mesh.name.c_str(), mesh.vertices, mesh.indices));
}

MaterialHandle_t makeMaterial(IRenderer &renderer, const char *name,
                              EShadingModel shading, const glm::vec4 &color,
                              EBlendMode blend) {
  return renderer.createMaterial({.debugName = name,
                                  .shading = shading,
                                  .blend = blend,
                                  .baseColor = color,
                                  .castShadow = blend == EBlendMode::kOpaque});
}

MaterialHandle_t litMaterial(IRenderer &renderer, const char *name,
                             const glm::vec4 &color) {
  return makeMaterial(renderer, name, EShadingModel::kLit, color);
}

} // namespace game::scene
