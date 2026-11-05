#pragma once

#include "engine/render/passes/render_pass.hpp"

namespace game {

// Sun ortho depth map (D32). When shadows are off, the map is only cleared to 1.0.
class CShadowPass final : public IRenderPass {
public:
    [[nodiscard]] bool create(const PassInitContext_t& context) override;
    void destroy(rhi::IRhiDevice& device) override;
    void execute(const FrameContext_t& context) override;

private:
    rhi::PipelineHandle_t m_pipeline;         // kPosNormalUv
    rhi::PipelineHandle_t m_skinnedPipeline;  // kPosNormalUvSkin
};

}  // namespace game
