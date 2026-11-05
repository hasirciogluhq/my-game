#include "client/scene/render_helpers.hpp"
#include "client/scene/scene.hpp"
#include "engine/engine/core/primitives.hpp"
#include "engine/interface/i_log.hpp"
#include "game/shared/level.hpp"

#include <limits>
#include <span>
#include <string_view>
#include <vector>

#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>
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

// Bind pozundaki modeli 1.8 m boyuna ölçekler, ayak merkezini orijine koyar.
glm::mat4 fitSkinned(const anim::SkinnedModel_t &model, float height) {
  std::vector<glm::mat4> skin;
  anim::evaluate(model, {}, {}, 0.0f, skin);
  glm::vec3 low{std::numeric_limits<float>::max()};
  glm::vec3 high{std::numeric_limits<float>::lowest()};
  for (const anim::SkinnedVertex_t &vertex : model.vertices) {
    glm::mat4 blended{0.0f};
    for (int i = 0; i < 4; ++i) {
      blended += skin[vertex.joints[static_cast<size_t>(i)]] * vertex.weights[i];
    }
    const glm::vec3 position = blended * glm::vec4{vertex.position, 1.0f};
    low = glm::min(low, position);
    high = glm::max(high, position);
  }
  const float scale = height / (high.y - low.y);
  const glm::vec3 baseCenter{(low.x + high.x) * 0.5f, low.y,
                             (low.z + high.z) * 0.5f};
  return glm::scale(glm::mat4{1.0f}, glm::vec3{scale}) *
         glm::translate(glm::mat4{1.0f}, -baseCenter);
}

// Klip adı yoksa fallback.
int clipOrElse(const anim::SkinnedModel_t &model, std::string_view name,
               int fallback) {
  const int clip = anim::findClip(model, name);
  return clip >= 0 ? clip : fallback;
}

// İskeletli modeli yükler, GPU'ya yollar; EAnimState -> klip eşlemesini adlardan kurar.
// Modelin dokusu varsa material onu kullanır. Klip adı bulunamazsa idle = ilk klip, greet = idle.
SkinnedAsset_t uploadSkinned(IAssets &assets, IRenderer &renderer,
                             const std::string &path, MaterialDesc_t material,
                             std::string_view greetClip, float yawOffset,
                             bool fit) {
  auto model = assets.loadSkinned(path);
  if (!model) {
    GAME_LOG_ERROR("client", "iskeletli model yüklenemedi: {}: {}", path,
                   model.error());
    return {};
  }
  const anim::TextureData_t &image = (*model)->baseColor;
  if (!image.pixels.empty()) {
    material.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    material.baseColorTexture = renderer.createTexture(
        {.debugName = "skinned_base_color",
         .width = image.width,
         .height = image.height,
         .pixels = std::as_bytes(std::span{image.pixels})});
  }
  SkinnedAsset_t asset{.model = *model,
                       .mesh = uploadSkinnedMesh(renderer, **model),
                       .material = renderer.createMaterial(material),
                       .yawOffset = yawOffset};
  if (fit) {
    asset.local = fitSkinned(**model, 1.8f);
  }
  const int idle = clipOrElse(**model, "idle", (*model)->clips.empty() ? -1 : 0);
  asset.clipOfState[static_cast<size_t>(EAnimState::kIdle)] = idle;
  asset.clipOfState[static_cast<size_t>(EAnimState::kGreet)] =
      clipOrElse(**model, greetClip, idle);
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
  resources.spot = uploadModel(assets, renderer, "models/spot.glb", 1.6f);
  resources.suzanne = uploadModel(assets, renderer, "models/suzanne.glb", 1.8f);
  resources.player = makeMaterial(renderer, "player", EShadingModel::kRimLit,
                                  {0.2f, 0.6f, 1.0f, 1.0f});
  resources.robot = uploadSkinned(
      assets, renderer, "models/robot.glb",
      {.debugName = "robot",
       .shading = EShadingModel::kRimLit,
       .baseColor = {0.95f, 0.55f, 0.15f, 1.0f}},
      "wave", 0.0f, false);
  // Gerçek karakter: klasörde character.* + idle/breakdance klipleri. Mixamo/glTF +Z'ye bakar, oyun -Z.
  resources.dancer = uploadSkinned(
      assets, renderer, "models/dancer",
      {.debugName = "dancer", .shading = EShadingModel::kLit},
      "breakdance", glm::pi<float>(), true);
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
