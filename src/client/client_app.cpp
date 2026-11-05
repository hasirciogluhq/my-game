#include "client/client_app.hpp"

#include "client/console/console_commands.hpp"
#include "game/client/camera/death_cam.hpp"
#include "game/client/effects/fire_effects.hpp"
#include "game/client/ui/console_window.hpp"
#include "game/client/ui/documents.hpp"
#include "game/client/ui/hud.hpp"
#include "game/client/ui/pause_menu.hpp"
#include "game/client/ui/scoreboard.hpp"
#include "game/shared/level.hpp"
#include "game/shared/movement.hpp"
#include "game/shared/snapshot.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <thread>

#include <imgui.h>

#include <glm/geometric.hpp>

namespace game {
namespace {

constexpr float kFreeCamSpeed = 8.0f;
constexpr float kFovDegrees = 70.0f;
constexpr float kNearPlane = 0.05f;
constexpr float kFarPlane = 500.0f;

} // namespace

void CClientApp::applyMap(EMap map) {
  scene::spawnStaticProps(m_world, *m_renderer, m_resources, level::boxes(map));
  if (m_physics != nullptr) {
    level::buildPhysics(*m_physics, map);
  }
  GAME_LOG_INFO("client", "map {}", mapName(map));
}

bool CClientApp::connect(CreateInterfaceFn factory) {
  m_platform = queryInterface<IPlatform>(factory, kPlatformInterface);
  m_renderer = queryInterface<IRenderer>(factory, kRendererInterface);
  m_particles = queryInterface<IParticles>(factory, kParticlesInterface);
  m_assets = queryInterface<IAssets>(factory, kAssetsInterface);
  m_audio = queryInterface<IAudio>(factory, kAudioInterface); // optional
  m_ui = queryInterface<IUI>(factory, kUIInterface);          // optional
  auto *networkFactory = queryInterface<INetworkFactory>(factory, kNetworkFactoryInterface);
  INetwork *network = networkFactory != nullptr ? networkFactory->createNetwork() : nullptr;
  if (m_platform == nullptr || m_renderer == nullptr ||
      m_particles == nullptr || m_assets == nullptr || network == nullptr) {
    GAME_LOG_ERROR("client", "platform/renderer/particles/assets/network "
                             "interfaces was not found");
    return false;
  }
  m_network.bind(network);
  if (m_audio != nullptr && !m_audio->initialize()) {
    m_audio = nullptr;
  }
  // Own physics world for prediction (optional: without it the client predicts on flat ground).
  auto *physicsFactory = queryInterface<IPhysicsFactory>(factory, kPhysicsFactoryInterface);
  m_physics = physicsFactory != nullptr ? physicsFactory->createWorld() : nullptr;
  if (m_physics != nullptr && !m_physics->initialize()) {
    m_physics = nullptr;
  }
  m_prediction.physics = m_physics;
  return true;
}

int CClientApp::run(const LaunchOptions_t &options) {
  if (m_platform == nullptr || m_renderer == nullptr) {
    GAME_LOG_ERROR("client", "run() cannot be called without connect()");
    return 1;
  }
  if (!m_platform->createWindow("my-game", 1280, 720)) {
    return 1;
  }
  const RendererDesc_t rendererDesc{
      .surface = {.kind = ENativeWindowKind::kSdlWindow,
                  .handle = m_platform->nativeWindow()}};
  if (!m_renderer->initialize(rendererDesc)) {
    GAME_LOG_ERROR("client", "renderer could not be started: {}",
                   m_renderer->lastError());
    return 1;
  }

  if (m_ui != nullptr && !initUi()) {
    m_ui = nullptr; // konsolsuz devam
  }

  auto resources = scene::loadResources(*m_assets, *m_renderer);
  if (!resources) {
    GAME_LOG_ERROR("client", "base meshes could not be loaded: {}",
                   m_renderer->lastError());
    return 1;
  }
  m_resources = std::move(*resources);
  world::reset(m_world, options.tickRate);
  // The map's boxes and physics are built when WELCOME arrives (applyMap), not here.
  m_world.physics = m_physics; // only for the local player's prediction

  if (!m_network.connectTo(options.host, options.port)) {
    GAME_LOG_ERROR("client", "could not connect to {}:{}", options.host, options.port);
    return 1;
  }
  // Key bindings: the defaults first, then assets/config/input.cfg. If the file is missing,
  // the defaults are written to it.
  m_bindings = input::defaultBindings();
  const std::filesystem::path bindingsPath = input::configPath();
  if (std::filesystem::exists(bindingsPath)) {
    input::load(m_bindings, bindingsPath);
  } else {
    input::save(m_bindings, bindingsPath);
  }
  m_input.bind(m_platform, &m_bindings);
  m_platform->setRelativeMouse(true);
  ui::print(m_console, "console: type 'help'");

  using clock = std::chrono::steady_clock;
  auto nextTick = clock::now();
  uint32_t tick = 0;
  uint32_t appliedRate = 0;
  clock::duration period{};

  while (true) {
    const PlatformEvents_t events = m_platform->pollEvents();
    updateConsole();
    if (events.quit || m_quitRequested) {
      break;
    }
    m_input.addMouseDelta(events.mouseDeltaX, events.mouseDeltaY);
    m_network.poll();
    if (const auto movement = m_network.movement()) {
      m_world.movement = *movement;  // the server's move.* cvars, from WELCOME
    }
    if (!m_mapApplied) {
      if (const auto map = m_network.map()) {
        applyMap(*map);
        m_mapApplied = true;
      }
    }

    // The server states the tick rate in the welcome packet; the default is used until then.
    const uint32_t rate = m_network.tickRate().value_or(kDefaultTickRate);
    if (rate != appliedRate) {
      appliedRate = rate;
      m_dt = tickDeltaFor(rate);
      period = std::chrono::duration_cast<clock::duration>(
          std::chrono::duration<double>(m_dt));
      nextTick = clock::now();
    }

    tickOnce(++tick);

    nextTick += period;
    const auto now = clock::now();
    if (nextTick > now) {
      std::this_thread::sleep_until(nextTick);
    } else {
      nextTick = now;
    }
  }

  m_network.disconnect();
  if (m_audio != nullptr) {
    m_audio->shutdown();
  }
  if (m_ui != nullptr) {
    m_platform->setEventHook(nullptr, nullptr);
    m_ui->shutdown();
  }
  m_renderer->shutdown();
  m_platform->destroyWindow();
  return 0;
}

// Binds ui.dll's ImGui context to this DLL (each DLL has its own ImGui copy),
// and sets up the event hook and the overlay.
bool CClientApp::initUi() {
  if (!m_ui->initialize(m_platform->nativeWindow(),
                        m_renderer->nativeDevice())) {
    GAME_LOG_WARN("client", "ui could not be started, no console");
    return false;
  }
  void *alloc = nullptr;
  void *free = nullptr;
  void *user = nullptr;
  m_ui->allocatorFunctions(&alloc, &free, &user);
  ImGui::SetAllocatorFunctions(reinterpret_cast<ImGuiMemAllocFunc>(alloc),
                               reinterpret_cast<ImGuiMemFreeFunc>(free), user);
  ImGui::SetCurrentContext(static_cast<ImGuiContext *>(m_ui->context()));
  m_platform->setEventHook(
      [](const void *event, void *ui) {
        static_cast<IUI *>(ui)->processEvent(event);
      },
      m_ui);
  m_renderer->setOverlay(m_ui->overlay(), m_ui->overlayUser());
  if (!ui::loadDocuments(*m_ui)) {
    GAME_LOG_WARN("client", "RmlUi documents could not be loaded: no HUD/menu/scoreboard");
  }
  return true;
}

// Pause menu: while open, the mouse is free and game input stops (same logic as the console focus).
void CClientApp::setMenuOpen(bool open) {
  m_menuOpen = open;
  ui::showPauseMenu(*m_ui, open);
  m_platform->setRelativeMouse(!open);
  m_input.setEnabled(!open);
}

void CClientApp::updateMenu() {
  ui::PauseResult_t result;
  while (const auto action = m_ui->nextAction()) {
    ui::handlePauseAction(*action, m_input, result);
  }
  if (result.quit) {
    m_quitRequested = true;
  }
  if (result.resume && m_menuOpen) {
    setMenuOpen(false);
  }
}

// When the console opens, focus is on the console; when it closes, focus returns to the game.
void CClientApp::setConsoleOpen(bool open) {
  m_console.open = open;
  setConsoleFocus(open);
}

// Focus on the console: mouse free, typing goes to the console, game input stops.
// Focus on the game: mouse locked, the console stays on screen but the game runs normally.
void CClientApp::setConsoleFocus(bool focused) {
  m_console.focused = focused;
  m_console.focusInput = focused;
  m_platform->setRelativeMouse(!focused);
  m_input.setEnabled(!focused);
}

// Opens the ImGui frame, draws the UI layers in order (HUD, scoreboard,
// console), opens/closes the console, and runs the command received on Enter.
void CClientApp::updateConsole() {
  if (m_ui == nullptr) {
    return;
  }
  m_ui->beginFrame();
  // Draw order (bottom to top): HUD -> scoreboard -> console.
  ui::updateHud(*m_ui, m_world, selfEntity());
  ui::updateScoreboard(
      *m_ui, m_world, selfEntity(),
      !m_menuOpen &&
          input::isDown(*m_platform, m_bindings, EAction::kScoreboard));
  updateMenu();
  const bool wasOpen = m_console.open;
  if (input::wasPressed(*m_platform, m_bindings, EAction::kConsole)) {
    if (m_menuOpen) {
      setMenuOpen(false);
    }
    setConsoleOpen(!wasOpen);
  } else if (m_platform->keyPressedByName("Escape")) {
    if (wasOpen) {
      setConsoleOpen(false);
    } else {
      setMenuOpen(!m_menuOpen);
    }
  }

  // Clicking outside the window locks the mouse to the game; Enter gives focus back
  // to the console.
  if (wasOpen && m_console.open) {
    if (m_console.focused && m_platform->keyPressedByName("Mouse1") &&
        !m_ui->wantsMouse()) {
      setConsoleFocus(false);
    } else if (!m_console.focused && m_platform->keyPressedByName("Return")) {
      setConsoleFocus(true);
    }
  }

  if (const auto line = ui::draw(m_console)) {
    console::Context_t context{m_console,    m_bindings,      m_input,
                               m_prediction, m_interpolation, m_quitRequested,
                               m_network,    m_world};
    console::execute(context, *line);
  }
}

void CClientApp::tickOnce(uint32_t tick) {
  // Free-cam: toggled by pressing the action key (edge detection). Does not run while the console is open.
  const bool freeCamDown =
      !m_console.focused && !m_menuOpen &&
      input::isDown(*m_platform, m_bindings, EAction::kFreeCam);
  if (freeCamDown && !m_freeCamKeyWasDown) {
    m_freeCam = !m_freeCam;
    if (m_freeCam) {
      m_freeCamPosition = m_camera.position();
    }
  }
  m_freeCamKeyWasDown = freeCamDown;

  const UserCmd_t cmd = m_input.sample(tick);

  // In free-cam, movement and buttons do not go to the server; only the view angles do.
  UserCmd_t outgoing = cmd;
  if (m_freeCam) {
    outgoing.forwardMove = 0.0f;
    outgoing.sideMove = 0.0f;
    outgoing.buttons = 0;
    outgoing.buttonsPressed = 0;
    outgoing.buttonsReleased = 0;
  }
  m_network.sendCommand(outgoing);

  // Move the local player immediately (the server processes the same command later);
  // reconcile corrects it when the snapshot arrives.
  entt::entity self = selfEntity();
  if (self != m_predictedPawn) {
    prediction::clear(m_prediction); // switched to another pawn (takeover /
                                     // watching): the old prediction is invalid
    m_predictedPawn = self;
  }
  if (self != entt::null) {
    prediction::predict(m_world, m_prediction, self, outgoing);
  }

  fire_effects::update(cmd, m_freeCam, *m_particles, m_audio,
                       m_camera.position(), m_camera.forward());

  // The newest snapshot is written to the world (create/destroy/health); all of them go
  // into the interpolation buffer.
  auto snapshots = m_network.consumeSnapshots();
  if (!snapshots.empty()) {
    replication::apply(m_world, snapshots.back());
    for (WorldSnapshot_t &snapshot : snapshots) {
      interpolation::push(m_interpolation, std::move(snapshot));
    }
    self = selfEntity(); // it may have just been created by the first snapshot
    if (self != entt::null) {
      prediction::reconcile(m_world, m_prediction, self);
    }
  }
  prediction::decay(m_prediction, m_dt);

  // Remote players and projectiles are shown at their past (renderTick) position;
  // the local player is not touched.
  interpolation::advance(m_interpolation, m_world.tickRate);
  interpolation::apply(m_world, m_interpolation,
                       m_network.playerId().value_or(0));
  m_particles->update(m_dt);

  // Client systems.
  scene::attachVisuals(m_world, m_resources);
  scene::spin(m_world, m_dt);
  systems::projectileTransforms(
      m_world, interpolation::displayTick(m_interpolation, m_world.tick));

  updateCamera(cmd);
  drawWorld();

  if (const auto id = m_network.playerId()) {
    glm::vec3 pos{0.0f};
    if (self != entt::null) {
      pos = m_world.registry.get<Transform_t>(self).position;
    }
    const std::string title = std::format(
        "my-game | id {} | tick {} | pos {:.1f} {:.1f} {:.1f} | {} | err "
        "{:.3f} | particles {}",
        *id, m_world.tick, pos.x, pos.y, pos.z,
        m_freeCam ? "FREECAM" : "player", glm::length(m_prediction.errorOffset),
        m_particles->count());
    m_platform->setTitle(title.c_str());
  }
}

void CClientApp::updateCamera(const UserCmd_t &cmd) {
  const glm::ivec2 size = m_platform->drawableSize();
  const float aspect =
      size.y > 0 ? static_cast<float>(size.x) / static_cast<float>(size.y)
                 : 1.0f;
  m_camera.setPerspective(kFovDegrees, aspect, kNearPlane, kFarPlane);
  m_camera.setAngles(cmd.yaw, cmd.pitch);

  if (m_freeCam) {
    // Horizontal movement: the horizontal projection of the view direction. Up/down: space / C.
    glm::vec3 forward = m_camera.forward();
    forward.y = 0.0f;
    if (glm::length(forward) > 0.0f) {
      forward = glm::normalize(forward);
    }
    const glm::vec3 right = m_camera.right();

    glm::vec3 move{0.0f};
    if (!m_console.focused) {
      const auto down = [&](EAction action) {
        return input::isDown(*m_platform, m_bindings, action);
      };
      if (down(EAction::kForward))
        move += forward;
      if (down(EAction::kBack))
        move -= forward;
      if (down(EAction::kRight))
        move += right;
      if (down(EAction::kLeft))
        move -= right;
      if (down(EAction::kJump))
        move += glm::vec3{0.0f, 1.0f, 0.0f};
      if (down(EAction::kCrouch))
        move -= glm::vec3{0.0f, 1.0f, 0.0f};
    }

    if (glm::length(move) > 0.0f) {
      m_freeCamPosition += glm::normalize(move) * kFreeCamSpeed * m_dt;
    }
    m_camera.setPosition(m_freeCamPosition);
    return;
  }

  if (const entt::entity self = selfEntity(); self != entt::null) {
    const glm::vec3 feet = m_world.registry.get<Transform_t>(self).position;
    m_camera.setPosition(feet + m_prediction.errorOffset +
                         glm::vec3{0.0f, kEyeHeight, 0.0f});
  } else if (const entt::entity watched = watchedEntity();
             watched != entt::null) {
    // Watching: from the watched pawn's eye, with its view angles (its position
    // comes from interpolation).
    const auto &transform = m_world.registry.get<Transform_t>(watched);
    m_camera.setAngles(transform.yaw, transform.pitch);
    m_camera.setPosition(transform.position +
                         glm::vec3{0.0f, kEyeHeight, 0.0f});
  } else if (m_network.playerId() && !m_world.byNetId.empty()) {
    // No pawn of mine (I died): watch the map from above, slowly orbiting.
    death_cam::update(m_deathCam, m_dt);
    const death_cam::Pose_t pose = death_cam::pose(m_deathCam, m_world);
    m_camera.setAngles(pose.yaw, pose.pitch);
    m_camera.setPosition(pose.position);
  }
}

entt::entity CClientApp::selfEntity() const {
  const auto id = m_network.playerId();
  return id ? world::findPlayer(m_world, *id) : entt::entity{entt::null};
}

entt::entity CClientApp::watchedEntity() const {
  const uint32_t netId = m_network.viewTargetNetId();
  return netId != 0 ? world::findByNetId(m_world, netId)
                    : entt::entity{entt::null};
}

// The pawn the camera is inside: our own character or the one we are watching.
entt::entity CClientApp::cameraEntity() const {
  const entt::entity self = selfEntity();
  return self != entt::null ? self : watchedEntity();
}

void CClientApp::drawWorld() {
  m_time += m_dt;
  m_renderer->beginFrame({.timeSeconds = m_time, .deltaSeconds = m_dt});
  m_renderer->setView(
      {.view = m_camera.view(), .projection = m_camera.projection()});
  m_renderer->setEnvironment({});
  m_renderer->setPostProcess({});
  scene::draw(m_world, *m_renderer,
              m_freeCam ? entt::entity{entt::null} : cameraEntity());
  scene::drawSkinned(m_world, *m_renderer, m_resources,
                     interpolation::displayTick(m_interpolation, m_world.tick));

  // For now, particles live outside the ECS: particles.dll provides the positions.
  scene::drawParticles(*m_renderer, m_resources, m_particles->livePositions());

  if (m_ui != nullptr) {
    m_ui->endFrame(); // ImGui::Render; the renderer's overlay does the drawing
  }
  m_renderer->endFrame();
}

} // namespace game
