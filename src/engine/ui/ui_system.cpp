#include "engine/ui/ui_system.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>

#include "engine/interface/i_log.hpp"

namespace game {

namespace {

// Renderer'ın overlay geçidi: pass == nullptr iken yükleme (render pass dışı),
// pass verilince çizim.
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
    GAME_LOG_ERROR("ui", "pencere veya GPU aygıtı yok");
    return false;
  }
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;
  ImGui::StyleColorsDark();
  if (!ImGui_ImplSDL3_InitForSDLGPU(window)) {
    GAME_LOG_ERROR("ui", "imgui SDL3 backend başlatılamadı");
    ImGui::DestroyContext();
    return false;
  }
  ImGui_ImplSDLGPU3_InitInfo info{};
  info.Device = device;
  info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
  info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
  if (!ImGui_ImplSDLGPU3_Init(&info)) {
    GAME_LOG_ERROR("ui", "imgui SDL_GPU backend başlatılamadı");
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return false;
  }
  m_initialized = true;
  if (!m_rml.initialize(window, device)) {
    GAME_LOG_WARN(
        "ui",
        "RmlUi başlatılamadı: oyun arayüzü (menü/HUD) yok, ImGui devam ediyor");
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
  // ImGui (konsol) bir girdiyi istiyorsa RmlUi o girdiyi görmez.
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
    m_rml.update(); // oyun bu frame'deki eleman değişikliklerini yaptı; şimdi
                    // güncelle ve çizim komutlarını kaydet
  }
}

bool CUiSystem::wantsKeyboard() const {
  return m_initialized && ImGui::GetIO().WantCaptureKeyboard;
}

bool CUiSystem::wantsMouse() const {
  return m_initialized && ImGui::GetIO().WantCaptureMouse;
}

RenderOverlayFn CUiSystem::overlay() const { return &drawOverlay; }

// Sıra: RmlUi (alt katman), ImGui (üstte: konsol HUD'un üstünü kaplar).
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
