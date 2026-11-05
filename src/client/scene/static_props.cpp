#include "client/scene/render_helpers.hpp"
#include "client/scene/scene.hpp"
#include "game/shared/level.hpp"

namespace game::scene {
namespace {

// Yerel entity: Transform + Visual (+ ops. Spin).
entt::entity spawnVisual(World_t &world, glm::vec3 position, float yaw,
                         const Visual_t &visual) {
  const entt::entity entity = world.registry.create();
  world.registry.emplace<Transform_t>(entity, position, yaw);
  world.registry.emplace<Visual_t>(entity, visual);
  return entity;
}

} // namespace

void spawnStaticProps(World_t &world, IRenderer &renderer,
                      const Resources_t &resources) {
  spawnVisual(world, {0.0f, 0.0f, 0.0f}, 0.0f,
              {.mesh = resources.plane,
               .material = litMaterial(renderer, "ground",
                                       {0.45f, 0.5f, 0.42f, 1.0f})});

  // Level kutuları: server'daki fizik kutularıyla aynı liste.
  for (const level::Box_t &box : level::boxes()) {
    spawnVisual(world, {0.0f, 0.0f, 0.0f}, 0.0f,
                {.mesh = resources.box,
                 .material = litMaterial(renderer, "level-box", box.color),
                 .local = scaleAt(box.center, box.halfExtents * 2.0f)});
  }

  // Model prop'ları: kaide üstünde Suzanne, dönen Spot'lar.
  if (resources.suzanne.mesh) {
    spawnVisual(world, {3.5f, 1.0f, -7.0f}, 0.0f,
                {.mesh = resources.suzanne.mesh,
                 .material = litMaterial(renderer, "suzanne",
                                         {0.8f, 0.45f, 0.2f, 1.0f}),
                 .local = resources.suzanne.local});
  }
  if (resources.spot.mesh) {
    const entt::entity spinning =
        spawnVisual(world, {-3.5f, 0.0f, -7.0f}, 0.0f,
                    {.mesh = resources.spot.mesh,
                     .material = litMaterial(renderer, "spot-white",
                                             {0.95f, 0.95f, 0.9f, 1.0f}),
                     .local = resources.spot.local});
    world.registry.emplace<Spin_t>(spinning, 0.6f);
    spawnVisual(world, {6.0f, 0.0f, -3.0f}, 2.2f,
                {.mesh = resources.spot.mesh,
                 .material = litMaterial(renderer, "spot-orange",
                                         {0.9f, 0.55f, 0.35f, 1.0f}),
                 .local = resources.spot.local});
  }
}

} // namespace game::scene
