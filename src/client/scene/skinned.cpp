#include "client/scene/scene.hpp"

#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace game::scene {
namespace {

const SkinnedAsset_t *assetFor(const Resources_t &resources, EModel model) {
  switch (model) {
  case EModel::kRobot:
    return &resources.robot;
  }
  return nullptr;
}

// state'in klibi, tick'ten (gösterim zamanı) beri geçen süreyle.
anim::ClipSample_t sampleOf(const SkinnedAsset_t &asset, EAnimState state,
                            uint32_t startTick, double atTick, float dt) {
  return {.clip = asset.clipOfState[static_cast<size_t>(state)],
          .seconds = static_cast<float>((atTick - startTick) * dt)};
}

} // namespace

void drawSkinned(World_t &world, IRenderer &renderer,
                 const Resources_t &resources, double atTick) {
  const float dt = world::tickDelta(world);
  std::vector<glm::mat4> skin; // submit() kopyalar; entity'ler arası yeniden kullanılır
  for (const auto [entity, transform, animator] :
       world.registry.view<const Transform_t, const Animator_t>().each()) {
    const SkinnedAsset_t *asset = assetFor(resources, animator.model);
    if (asset == nullptr || asset->model == nullptr || !asset->mesh) {
      continue;
    }
    const anim::ClipSample_t to =
        sampleOf(*asset, animator.state, animator.startTick, atTick, dt);
    const anim::ClipSample_t from = sampleOf(
        *asset, animator.prevState, animator.prevStartTick, atTick, dt);
    anim::evaluate(*asset->model, from, to, to.seconds / kAnimBlendSeconds,
                   skin);

    const glm::mat4 model =
        glm::translate(glm::mat4{1.0f}, transform.position) *
        glm::rotate(glm::mat4{1.0f}, transform.yaw,
                    glm::vec3{0.0f, 1.0f, 0.0f});
    renderer.submit({.mesh = asset->mesh,
                     .material = asset->material,
                     .transform = model,
                     .bones = skin});
  }
}

} // namespace game::scene
