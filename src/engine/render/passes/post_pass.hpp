#pragma once

#include "engine/render/passes/render_pass.hpp"

namespace game {

// HDR -> swapchain: bloom, tonemap, vignette, gamma (swapchain sRGB değilse). Pipeline backbuffer formatına bağlıdır.
class CPostPass final : public IRenderPass {
public:
    [[nodiscard]] bool create(const PassInitContext_t& context) override;
    void destroy(rhi::IRhiDevice& device) override;
    void execute(const FrameContext_t& context) override;

private:
    CShaderLibrary* m_shaders = nullptr;
    rhi::PipelineHandle_t m_pipeline;
    rhi::EPixelFormat m_pipelineFormat = rhi::EPixelFormat::kUndefined;
};

}  // namespace game
