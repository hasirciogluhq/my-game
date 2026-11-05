#pragma once

#include "engine/render/passes/render_pass.hpp"

namespace game {

// Güneş ortho derinlik haritası (D32). Gölge kapalıyken harita sadece 1.0'a temizlenir.
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
