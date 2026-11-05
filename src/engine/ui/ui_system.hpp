#pragma once

#include "engine/interface/i_ui.hpp"
#include "engine/ui/api.hpp"

namespace game {

// UI001 uygulaması: Dear ImGui + resmi SDL3 / SDL_GPU3 backend'leri. Başka çizim kodu yoktur.
class UI_API CUiSystem final : public IUI {
public:
    CUiSystem() = default;
    CUiSystem(const CUiSystem&) = delete;
    CUiSystem& operator=(const CUiSystem&) = delete;
    ~CUiSystem() override;

    [[nodiscard]] bool initialize(void* sdlWindow, void* sdlGpuDevice) override;
    void shutdown() override;

    void processEvent(const void* event) override;
    void beginFrame() override;
    void endFrame() override;

    [[nodiscard]] bool wantsKeyboard() const override;
    [[nodiscard]] bool wantsMouse() const override;

    [[nodiscard]] RenderOverlayFn overlay() const override;
    [[nodiscard]] void* overlayUser() override { return this; }

    [[nodiscard]] void* context() const override;
    void allocatorFunctions(void** alloc, void** free, void** user) const override;

private:
    bool m_initialized = false;
};

}  // namespace game
