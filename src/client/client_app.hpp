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

// Client main loop. The platform, renderer, particles, assets and audio interfaces
// come from the host. Each tick: events -> input -> send to the server -> apply the snapshot
// to the world -> camera -> draw. Camera: normally the eye level of the player on the server.
// The F key toggles free-cam (local only; no movement is sent to the server).
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
  // Builds the server's map locally: draws its boxes and adds them to the prediction physics.
  void applyMap(EMap map);
  void drawWorld();
  [[nodiscard]] entt::entity
  selfEntity() const; // the pawn we are driving right now, or entt::null
  [[nodiscard]] entt::entity
  watchedEntity() const; // the pawn we are watching, or entt::null
  [[nodiscard]] entt::entity cameraEntity() const;

  IPlatform *m_platform = nullptr;
  IRenderer *m_renderer = nullptr;
  IParticles *m_particles = nullptr;
  IAssets *m_assets = nullptr;
  IAudio *m_audio = nullptr;     // optional: silent if missing
  ui::Console_t m_console; // ImGui console (focused: mouse free)
  IUI *m_ui = nullptr; // optional: no console if missing
  IPhysics *m_physics =
      nullptr; // optional: if missing, prediction runs without collision (flat ground)

  prediction::Prediction_t m_prediction;
  entt::entity m_predictedPawn = entt::null; // the pawn the prediction belongs to
  interpolation::Interpolation_t m_interpolation;
  CNetClient m_network;
  CInputSampler m_input;
  Bindings_t m_bindings; // action -> key (assets/config/input.cfg)
  bool m_quitRequested = false;
  bool m_menuOpen = false; // RmlUi pause menu (Esc)
  World_t
      m_world; // fed only from server snapshots + local props
  CCamera m_camera;
  death_cam::DeathCam_t m_deathCam;
  scene::Resources_t m_resources;
  bool m_mapApplied = false; // the server's map (WELCOME) has been built locally

  float m_dt = tickDeltaFor(kDefaultTickRate);
  float m_time = 0.0f;

  bool m_freeCam = false;
  bool m_freeCamKeyWasDown = false;
  glm::vec3 m_freeCamPosition{0.0f};
};

} // namespace game
