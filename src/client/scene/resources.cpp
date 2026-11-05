#include "client/scene/render_helpers.hpp"
#include "client/scene/scene.hpp"
#include "engine/engine/core/primitives.hpp"
#include "engine/interface/i_log.hpp"
#include "game/shared/level.hpp"

#include <limits>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace game::scene {
namespace {

// Modeli verilen boyda yere oturtur: ayak merkezi orijinde.
glm::mat4 fitToHeight(const MeshData_t &mesh, float height) {
  glm::vec3 low{std::numeric_limits<float>::max()};
  glm::vec3 high{std::numeric_limits<float>::lowest()};
  for (const Vertex_t &vertex : mesh.vertices) {
    low = glm::min(low, vertex.position);
    high = glm::max(high, vertex.position);
  }
  const float scale = height / (high.y - low.y);
  const glm::vec3 baseCenter{(low.x + high.x) * 0.5f, low.y,
                             (low.z + high.z) * 0.5f};
  return glm::scale(glm::mat4{1.0f}, glm::vec3{scale}) *
         glm::translate(glm::mat4{1.0f}, -baseCenter);
}

Model_t uploadModel(IAssets &assets, IRenderer &renderer,
                    const std::string &path, float height) {
  const auto mesh = assets.loadMesh(path);
  if (!mesh) {
    GAME_LOG_ERROR("client", "model yüklenemedi: {}: {}", path, mesh.error());
    return {};
  }
  GAME_LOG_INFO("client", "model yüklendi: {} ({} üçgen)", path,
                (*mesh)->indices.size() / 3);
  return {.mesh = uploadMesh(renderer, **mesh),
          .local = fitToHeight(**mesh, height)};
}

// İskeletli modeli yükler, GPU'ya yollar; EAnimState -> klip eşlemesini adlardan kurar.
SkinnedAsset_t uploadSkinned(IAssets &assets, IRenderer &renderer,
                             const std::string &path,
                             const MaterialHandle_t &material) {
  auto model = assets.loadSkinned(path);
  if (!model) {
    GAME_LOG_ERROR("client", "iskeletli model yüklenemedi: {}: {}", path,
                   model.error());
    return {};
  }
  SkinnedAsset_t asset{.model = *model,
                       .mesh = uploadSkinnedMesh(renderer, **model),
                       .material = material};
  asset.clipOfState[static_cast<size_t>(EAnimState::kIdle)] =
      anim::findClip(**model, "idle");
  asset.clipOfState[static_cast<size_t>(EAnimState::kWave)] =
      anim::findClip(**model, "wave");
  GAME_LOG_INFO("client", "iskeletli model yüklendi: {} ({} üçgen, {} eklem, {} klip)",
                path, (*model)->indices.size() / 3, (*model)->joints.size(),
                (*model)->clips.size());
  return asset;
}

} // namespace

std::optional<Resources_t> loadResources(IAssets &assets, IRenderer &renderer) {
  Resources_t resources;
  resources.plane =
      uploadMesh(renderer, makePlaneMesh("ground", level::kGroundHalfExtent));
  resources.box = uploadMesh(renderer, makeBoxMesh("box"));
  if (!resources.plane || !resources.box) {
    return std::nullopt;
  }
  resources.spot = uploadModel(assets, renderer, "models/spot.obj", 1.6f);
  resources.suzanne = uploadModel(assets, renderer, "models/suzanne.obj", 1.8f);
  resources.player = makeMaterial(renderer, "player", EShadingModel::kRimLit,
                                  {0.2f, 0.6f, 1.0f, 1.0f});
  resources.robot = uploadSkinned(
      assets, renderer, "models/robot.gltf",
      makeMaterial(renderer, "robot", EShadingModel::kRimLit,
                   {0.95f, 0.55f, 0.15f, 1.0f}));
  resources.projectile =
      renderer.createMaterial({.debugName = "projectile",
                               .shading = EShadingModel::kUnlit,
                               .baseColor = {1.0f, 0.9f, 0.2f, 1.0f},
                               .emissive = {2.0f, 1.6f, 0.3f},
                               .castShadow = false});
  resources.particle =
      renderer.createMaterial({.debugName = "particle",
                               .shading = EShadingModel::kUnlit,
                               .baseColor = {1.0f, 0.5f, 0.1f, 1.0f},
                               .emissive = {2.0f, 0.8f, 0.1f},
                               .castShadow = false});
  return resources;
}

} // namespace game::scene
