#pragma once

#include "client/api.hpp"
#include "client/i_client.hpp"
#include "game/client/ui/console_window.hpp"
#include "client/net/net_client.hpp"
#include "client/scene/scene.hpp"
#include "engine/engine/core/camera.hpp"
#include "engine/interface/i_assets.hpp"
#include "engine/interface/i_audio.hpp"
#include "engine/interface/i_particles.hpp"
#include "engine/interface/i_physics.hpp"
#include "engine/interface/i_platform.hpp"
#include "engine/interface/i_renderer.hpp"
#include "engine/interface/i_ui.hpp"
#include "game/client/camera/death_cam.hpp"
#include "game/client/input/input_map.hpp"
#include "game/client/input/input_sampler.hpp"
#include "game/client/interpolation/interpolation.hpp"
#include "game/client/prediction/prediction.hpp"
#include "game/shared/world.hpp"

#include <cstdint>

namespace game {

// Client ana döngüsü. Platform, renderer, particles, assets, audio arayüzleri
// host'tan gelir. Her tick: olaylar -> input -> server'a gönder -> snapshot'ı
// world'e uygula -> kamera -> çiz. Kamera: normalde server'daki oyuncunun göz
// hizası. F tuşu ile free-cam (sadece yerel, server'a hareket gitmez).
class CLIENT_API CClientApp final : public IClientApp {
public:
  [[nodiscard]] bool connect(CreateInterfaceFn factory) override;
  [[nodiscard]] int run(const LaunchOptions_t &options) override;

private:
  void tickOnce(uint32_t tick);
  [[nodiscard]] bool initUi();
  void updateConsole();
  void setConsoleOpen(bool open);
  void setConsoleFocus(bool focused);
  void setMenuOpen(bool open);
  void updateMenu();
  void updateCamera(const UserCmd_t &cmd);
  void drawWorld();
  [[nodiscard]] entt::entity
  selfEntity() const; // şu an sürdüğümüz pawn ya da entt::null
  [[nodiscard]] entt::entity
  watchedEntity() const; // izlediğimiz pawn ya da entt::null
  [[nodiscard]] entt::entity cameraEntity() const;

  IPlatform *m_platform = nullptr;
  IRenderer *m_renderer = nullptr;
  IParticles *m_particles = nullptr;
  IAssets *m_assets = nullptr;
  IAudio *m_audio = nullptr;     // opsiyonel: yoksa sessiz
  ui::Console_t m_console; // ImGui konsolu (focused: fare serbest)
  IUI *m_ui = nullptr; // opsiyonel: yoksa konsol yok
  IPhysics *m_physics =
      nullptr; // opsiyonel: yoksa prediction çarpışmasız (düz zemin)

  prediction::Prediction_t m_prediction;
  entt::entity m_predictedPawn = entt::null; // tahminin ait olduğu pawn
  interpolation::Interpolation_t m_interpolation;
  CNetClient m_network;
  CInputSampler m_input;
  Bindings_t m_bindings; // aksiyon -> tuş (assets/config/input.cfg)
  bool m_quitRequested = false;
  bool m_menuOpen = false; // RmlUi duraklatma menüsü (Esc)
  World_t
      m_world; // sadece server snapshot'larından + yerel prop'lardan beslenir
  CCamera m_camera;
  death_cam::DeathCam_t m_deathCam;
  scene::Resources_t m_resources;

  float m_dt = tickDeltaFor(kDefaultTickRate);
  float m_time = 0.0f;

  bool m_freeCam = false;
  bool m_freeCamKeyWasDown = false;
  glm::vec3 m_freeCamPosition{0.0f};
};

} // namespace game
