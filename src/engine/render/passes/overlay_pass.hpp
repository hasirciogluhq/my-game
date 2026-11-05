#pragma once

#include "engine/render/passes/render_pass.hpp"

namespace game {

// Final step: after post-processing, draws the externally registered overlay onto the swapchain (Dear ImGui's official
// SDL_GPU backend). The renderer does not know what the overlay draws; it only provides the command buffer and render pass.
class COverlayPass final : public IRenderPass {
public:
    [[nodiscard]] bool create(const PassInitContext_t& context) override;
    void destroy(rhi::IRhiDevice& device) override;
    void execute(const FrameContext_t& context) override;
};

}  // namespace game
