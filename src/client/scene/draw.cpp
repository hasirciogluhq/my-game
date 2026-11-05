#include "client/scene/scene.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace game::scene {

void draw(World_t &world, IRenderer &renderer, entt::entity hidden) {
  for (const auto [entity, transform, visual] :
       world.registry.view<const Transform_t, const Visual_t>().each()) {
    if (entity == hidden) {
      continue;
    }
    const glm::mat4 model =
        glm::translate(glm::mat4{1.0f}, transform.position) *
        glm::rotate(glm::mat4{1.0f}, transform.yaw,
                    glm::vec3{0.0f, 1.0f, 0.0f}) *
        visual.local;
    renderer.submit(
        {.mesh = visual.mesh, .material = visual.material, .transform = model});
  }
}

void drawParticles(IRenderer &renderer, const Resources_t &resources,
                   std::span<const glm::vec3> positions) {
  for (const glm::vec3 &position : positions) {
    const glm::mat4 model = glm::translate(glm::mat4{1.0f}, position) *
                            glm::scale(glm::mat4{1.0f}, glm::vec3{0.05f});
    renderer.submit({.mesh = resources.box,
                     .material = resources.particle,
                     .transform = model,
                     .castShadow = false});
  }
}

} // namespace game::scene
