#pragma once

#include "engine/render/passes/render_pass.hpp"

namespace game {

// Son adım: post-process'ten sonra swapchain'in üstüne dışarıdan kayıtlı overlay çizer (Dear ImGui'nin resmi
// SDL_GPU backend'i). Renderer overlay'in ne çizdiğini bilmez; sadece komut tamponunu ve render pass'i verir.
class COverlayPass final : public IRenderPass {
public:
    [[nodiscard]] bool create(const PassInitContext_t& context) override;
    void destroy(rhi::IRhiDevice& device) override;
    void execute(const FrameContext_t& context) override;
};

}  // namespace game
