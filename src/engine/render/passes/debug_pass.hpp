#pragma once

#include "engine/render/passes/render_pass.hpp"

namespace game {

// Depth-tested debug lines (over the HDR scene). The vertex buffer is rewritten every frame and grows when needed.
class CDebugPass final : public IRenderPass {
public:
    [[nodiscard]] bool create(const PassInitContext_t& context) override;
    void destroy(rhi::IRhiDevice& device) override;
    void execute(const FrameContext_t& context) override;

private:
    [[nodiscard]] bool ensureCapacity(rhi::IRhiDevice& device, uint32_t vertexCount);

    rhi::PipelineHandle_t m_pipeline;
    rhi::BufferHandle_t m_buffer;
    uint32_t m_capacityVertices = 0;
};

}  // namespace game
