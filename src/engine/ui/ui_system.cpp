#include "engine/ui/ui_system.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>

#include "engine/interface/i_log.hpp"

namespace game {

namespace {

// Overlay gate for the renderer: with pass == nullptr it uploads (outside a render pass),
// and when a pass is given it draws.
void drawOverlay(void *user, void *commandBuffer, void *renderPass) {
  static_cast<CUiSystem *>(user)->recordOverlay(commandBuffer, renderPass);
}

} // namespace

CUiSystem::~CUiSystem() { shutdown(); }

bool CUiSystem::initialize(void *sdlWindow, void *sdlGpuDevice) {
  if (m_initialized) {
    return true;
  }
  auto *window = static_cast<SDL_Window *>(sdlWindow);
  auto *device = static_cast<SDL_GPUDevice *>(sdlGpuDevice);
  if (window == nullptr || device == nullptr) {
    GAME_LOG_ERROR("ui", "no window or GPU device");
    return false;
  }
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;
  ImGui::StyleColorsDark();
  if (!ImGui_ImplSDL3_InitForSDLGPU(window)) {
    GAME_LOG_ERROR("ui", "imgui SDL3 backend could not be started");
    ImGui::DestroyContext();
    return false;
  }
  ImGui_ImplSDLGPU3_InitInfo info{};
  info.Device = device;
  info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
  info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
  if (!ImGui_ImplSDLGPU3_Init(&info)) {
    GAME_LOG_ERROR("ui", "imgui SDL_GPU backend could not be started");
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return false;
  }
  m_initialized = true;
  if (!m_rml.initialize(window, device)) {
    GAME_LOG_WARN(
        "ui",
        "RmlUi could not be started: no game UI (menu/HUD), ImGui continues");
  }
  return true;
}

void CUiSystem::shutdown() {
  if (!m_initialized) {
    return;
  }
  m_rml.shutdown();
  ImGui_ImplSDLGPU3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  m_initialized = false;
}

void CUiSystem::processEvent(const void *event) {
  if (!m_initialized) {
    return;
  }
  const auto &sdlEvent = *static_cast<const SDL_Event *>(event);
  ImGui_ImplSDL3_ProcessEvent(&sdlEvent);
  // If ImGui (the console) wants an input, RmlUi does not see it.
  m_rml.processEvent(sdlEvent, !wantsMouse(), !wantsKeyboard());
}

void CUiSystem::beginFrame() {
  if (m_initialized) {
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
  }
}

void CUiSystem::endFrame() {
  if (m_initialized) {
    ImGui::Render();
    m_rml.update(); // the game has made its element changes for this frame; now
                    // update and record the draw commands
  }
}

bool CUiSystem::wantsKeyboard() const {
  return m_initialized && ImGui::GetIO().WantCaptureKeyboard;
}

bool CUiSystem::wantsMouse() const {
  return m_initialized && ImGui::GetIO().WantCaptureMouse;
}

RenderOverlayFn CUiSystem::overlay() const { return &drawOverlay; }

// Order: RmlUi (bottom layer), ImGui (on top: the console covers the HUD).
void CUiSystem::recordOverlay(void *commandBuffer, void *renderPass) {
  auto *commands = static_cast<SDL_GPUCommandBuffer *>(commandBuffer);
  auto *pass = static_cast<SDL_GPURenderPass *>(renderPass);
  if (pass == nullptr) {
    m_rml.prepare(commands);
  } else {
    m_rml.draw(commands, pass);
  }
  ImDrawData *drawData = ImGui::GetDrawData();
  if (drawData == nullptr || drawData->TotalVtxCount == 0) {
    return;
  }
  if (pass == nullptr) {
    ImGui_ImplSDLGPU3_PrepareDrawData(drawData, commands);
  } else {
    ImGui_ImplSDLGPU3_RenderDrawData(drawData, commands, pass);
  }
}

void *CUiSystem::context() const { return ImGui::GetCurrentContext(); }

void CUiSystem::allocatorFunctions(void **alloc, void **free,
                                   void **user) const {
  ImGuiMemAllocFunc allocFn = nullptr;
  ImGuiMemFreeFunc freeFn = nullptr;
  ImGui::GetAllocatorFunctions(&allocFn, &freeFn, user);
  *alloc = reinterpret_cast<void *>(allocFn);
  *free = reinterpret_cast<void *>(freeFn);
}

} // namespace game
