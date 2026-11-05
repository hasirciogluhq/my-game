#pragma once

#include "engine/interface/render_types.hpp"

namespace game {

// ui.dll arayüzü: motorun tek UI sistemi Dear ImGui. Resmi backend'ler (imgui_impl_sdl3 + imgui_impl_sdlgpu3) ui.dll içinde.
// Oyun ImGui kodunu doğrudan yazar: client.dll da imgui'yi bağlar, context() ve allocator ile ui.dll'in bağlamını paylaşır.
//
// Frame akışı: platform olayları -> processEvent (setEventHook ile) -> beginFrame -> ImGui::* çağrıları -> endFrame.
// Çizimi renderer yapar: renderer->setOverlay(ui->overlay(), ui->overlayUser()) bir kez çağrılır.
class IUI {
public:
    virtual ~IUI() = default;

    // sdlWindow: SDL_Window*, sdlGpuDevice: SDL_GPUDevice* (IRenderer::nativeDevice). Renderer başlatıldıktan sonra çağrılır.
    [[nodiscard]] virtual bool initialize(void* sdlWindow, void* sdlGpuDevice) = 0;
    virtual void shutdown() = 0;

    // event: const SDL_Event* (IPlatform::setEventHook bu imzayı verir).
    virtual void processEvent(const void* event) = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    // ImGui klavye/fareyi istiyor mu? (oyun girdisini durdurmak için)
    [[nodiscard]] virtual bool wantsKeyboard() const = 0;
    [[nodiscard]] virtual bool wantsMouse() const = 0;

    [[nodiscard]] virtual RenderOverlayFn overlay() const = 0;
    [[nodiscard]] virtual void* overlayUser() = 0;

    // ImGui::SetCurrentContext(context) + ImGui::SetAllocatorFunctions(alloc, free, allocUser) için ham değerler (imgui türleri sızmaz).
    [[nodiscard]] virtual void* context() const = 0;
    virtual void allocatorFunctions(void** alloc, void** free, void** user) const = 0;
};

inline constexpr char kUIInterface[] = "UI001";

}  // namespace game
