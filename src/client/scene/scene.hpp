#pragma once

#include "engine/engine/core/camera.hpp"
#include "engine/interface/i_assets.hpp"
#include "engine/interface/i_renderer.hpp"
#include "game/shared/world.hpp"

#include "engine/engine/anim/skinned_model.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Client-only components and the functions that process them (ECS: data +
// system).
namespace game::scene {

// The entity to draw: Transform + Visual. world = T(position) * R(yaw) * local.
// Color and shading live in the material (player: kRimLit, bullet/particle: kUnlit).
struct Visual_t {
  MeshHandle_t mesh;
  MaterialHandle_t material;
  glm::mat4 local{1.0f};
};

struct Spin_t {
  float radiansPerSecond = 0.0f;
};

// This client's own player: not drawn in first-person view.

// A model loaded onto the GPU + a ground-placement matrix.
struct Model_t {
  MeshHandle_t mesh;
  glm::mat4 local{1.0f};
};

// A skinned model loaded onto the GPU. clipOfState: EAnimState -> clip index in the model (-1 = none, bind pose).
struct SkinnedAsset_t {
  std::shared_ptr<const anim::SkinnedModel_t> model;
  MeshHandle_t mesh;
  MaterialHandle_t material;
  glm::mat4 local{1.0f};  // scale + ground placement (in the model's own space)
  float yawOffset = 0.0f; // difference between the direction the model faces and the game direction (-Z)
  std::array<int, 2> clipOfState{-1, -1};
};

// Blend duration when changing from the previous animation to the new one.
inline constexpr float kAnimBlendSeconds = 0.25f;

// Meshes loaded onto the GPU and shared materials.
struct Resources_t {
  MeshHandle_t plane;
  MeshHandle_t box;
  Model_t spot;
  Model_t suzanne;
  SkinnedAsset_t robot;        // EModel::kRobot
  SkinnedAsset_t dancer;       // EModel::kDancer
  MaterialHandle_t player;     // kRimLit
  MaterialHandle_t projectile; // kUnlit
  MaterialHandle_t particle;   // kUnlit
};

// Local (not sent over the network) scene: ground, level boxes, model props.
// Loads the models from disk with Assets001 and sends them to the renderer's GPU.
[[nodiscard]] std::optional<Resources_t> loadResources(IAssets &assets,
                                                       IRenderer &renderer);
void spawnStaticProps(World_t &world, IRenderer &renderer,
                      const Resources_t &resources);

// Called every frame.
void attachVisuals(World_t &world, const Resources_t &resources);
void spin(World_t &world, float dt);
// hidden: the pawn the camera is inside (first person) is not drawn; if entt::null,
// all of them are drawn.
void draw(World_t &world, IRenderer &renderer, entt::entity hidden);
// Draws the entities that have an Animator_t. The pose is computed from the state + startTick chosen by the server and from atTick (display time);
// the client makes no animation decisions.
void drawSkinned(World_t &world, IRenderer &renderer,
                 const Resources_t &resources, double atTick);
void drawParticles(IRenderer &renderer, const Resources_t &resources,
                   std::span<const glm::vec3> positions);

} // namespace game::scene
