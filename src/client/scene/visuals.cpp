#include "client/scene/render_helpers.hpp"
#include "client/scene/scene.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace game::scene {

void attachVisuals(World_t &world, const Resources_t &resources) {
  auto &registry = world.registry;

  for (const auto [entity, player] :
       registry.view<const Player_t>(entt::exclude<Visual_t>).each()) {
    registry.emplace<Visual_t>(
        entity,
        Visual_t{.mesh = resources.box,
                 .material = resources.player,
                 .local = scaleAt({0.0f, 0.9f, 0.0f}, {0.6f, 1.8f, 0.6f})});
  }
  for (const auto [entity, projectile] :
       registry.view<const Projectile_t>(entt::exclude<Visual_t>).each()) {
    registry.emplace<Visual_t>(
        entity,
        Visual_t{.mesh = resources.box,
                 .material = resources.projectile,
                 .local = glm::scale(glm::mat4{1.0f}, glm::vec3{0.15f})});
  }
}

void spin(World_t &world, float dt) {
  for (const auto [entity, transform, spinner] :
       world.registry.view<Transform_t, const Spin_t>().each()) {
    transform.yaw += spinner.radiansPerSecond * dt;
  }
}

} // namespace game::scene
