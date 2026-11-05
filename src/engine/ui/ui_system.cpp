#include "engine/ui/ui_system.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>

#include "engine/interface/i_log.hpp"

namespace game {

namespace {

// Renderer'ın overlay geçidi: pass == nullptr iken yükleme (render pass dışı), pass verilince çizim.
void drawOverlay(void*, void* commandBuffer, void* renderPass) {
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData == nullptr || drawData->TotalVtxCount == 0) {
        return;
    }
    auto* commands = static_cast<SDL_GPUCommandBuffer*>(commandBuffer);
    if (renderPass == nullptr) {
        ImGui_ImplSDLGPU3_PrepareDrawData(drawData, commands);
    } else {
        ImGui_ImplSDLGPU3_RenderDrawData(drawData, commands, static_cast<SDL_GPURenderPass*>(renderPass));
    }
}

}  // namespace

CUiSystem::~CUiSystem() {
    shutdown();
}

bool CUiSystem::initialize(void* sdlWindow, void* sdlGpuDevice) {
    if (m_initialized) {
        return true;
    }
    auto* window = static_cast<SDL_Window*>(sdlWindow);
    auto* device = static_cast<SDL_GPUDevice*>(sdlGpuDevice);
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
    return true;
}

void CUiSystem::shutdown() {
    if (!m_initialized) {
        return;
    }
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void CUiSystem::processEvent(const void* event) {
    if (m_initialized) {
        ImGui_ImplSDL3_ProcessEvent(static_cast<const SDL_Event*>(event));
    }
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
    }
}

bool CUiSystem::wantsKeyboard() const {
    return m_initialized && ImGui::GetIO().WantCaptureKeyboard;
}

bool CUiSystem::wantsMouse() const {
    return m_initialized && ImGui::GetIO().WantCaptureMouse;
}

RenderOverlayFn CUiSystem::overlay() const {
    return &drawOverlay;
}

void* CUiSystem::context() const {
    return ImGui::GetCurrentContext();
}

void CUiSystem::allocatorFunctions(void** alloc, void** free, void** user) const {
    ImGuiMemAllocFunc allocFn = nullptr;
    ImGuiMemFreeFunc freeFn = nullptr;
    ImGui::GetAllocatorFunctions(&allocFn, &freeFn, user);
    *alloc = reinterpret_cast<void*>(allocFn);
    *free = reinterpret_cast<void*>(freeFn);
}

}  // namespace game
