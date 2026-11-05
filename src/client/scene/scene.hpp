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

// Client'a özel component'ler ve onları işleyen fonksiyonlar (ECS: veri +
// system).
namespace game::scene {

// Çizilecek entity: Transform + Visual. world = T(position) * R(yaw) * local.
// Renk ve gölgeleme material'dedir (oyuncu: kRimLit, mermi/parçacık: kUnlit).
struct Visual_t {
  MeshHandle_t mesh;
  MaterialHandle_t material;
  glm::mat4 local{1.0f};
};

struct Spin_t {
  float radiansPerSecond = 0.0f;
};

// Bu client'ın kendi oyuncusu: birinci şahıs görünümde çizilmez.

// GPU'ya yüklenmiş model + yere oturtma matrisi.
struct Model_t {
  MeshHandle_t mesh;
  glm::mat4 local{1.0f};
};

// GPU'ya yüklenmiş iskeletli model. clipOfState: EAnimState -> modeldeki klip indeksi (-1 = yok, bind pozu).
struct SkinnedAsset_t {
  std::shared_ptr<const anim::SkinnedModel_t> model;
  MeshHandle_t mesh;
  MaterialHandle_t material;
  glm::mat4 local{1.0f};  // ölçek + yere oturtma (modelin kendi uzayında)
  float yawOffset = 0.0f; // modelin baktığı yön ile oyun yönü (-Z) farkı
  std::array<int, 2> clipOfState{-1, -1};
};

// Durum değişince önceki animasyondan yenisine geçiş süresi.
inline constexpr float kAnimBlendSeconds = 0.25f;

// GPU'ya yüklenmiş mesh'ler ve ortak material'ler.
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

// Yerel (ağa gitmeyen) sahne: zemin, level kutuları, model prop'ları.
// Modelleri diskten Assets001 ile yükler, GPU'ya renderer'a yollar.
[[nodiscard]] std::optional<Resources_t> loadResources(IAssets &assets,
                                                       IRenderer &renderer);
void spawnStaticProps(World_t &world, IRenderer &renderer,
                      const Resources_t &resources);

// Her frame çağrılanlar.
void attachVisuals(World_t &world, const Resources_t &resources);
void spin(World_t &world, float dt);
// hidden: kameranın içinde olduğu pawn (birinci şahıs) çizilmez; entt::null ise
// hepsi çizilir.
void draw(World_t &world, IRenderer &renderer, entt::entity hidden);
// Animator_t'li entity'leri çizer. Poz, server'ın seçtiği state + startTick'ten ve atTick'ten (gösterim zamanı)
// hesaplanır; client animasyon kararı vermez.
void drawSkinned(World_t &world, IRenderer &renderer,
                 const Resources_t &resources, double atTick);
void drawParticles(IRenderer &renderer, const Resources_t &resources,
                   std::span<const glm::vec3> positions);

} // namespace game::scene
