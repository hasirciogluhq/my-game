#include "engine/render/passes/post_pass.hpp"

#include <utility>

namespace game {

bool CPostPass::create(const PassInitContext_t& context) {
    m_shaders = &context.shaders;
    return m_shaders->get(EShaderId::kPostFragment).valid();  // the pipeline is created once the backbuffer format is known
}

void CPostPass::destroy(rhi::IRhiDevice& device) {
    device.destroy(std::exchange(m_pipeline, {}));
    m_pipelineFormat = rhi::EPixelFormat::kUndefined;
}

void CPostPass::execute(const FrameContext_t& context) {
    const rhi::EPixelFormat format = context.backbuffer.format;
    if (!m_pipeline.valid() || m_pipelineFormat != format) {
        context.device.destroy(std::exchange(m_pipeline, {}));
        m_pipeline = context.device.createGraphicsPipeline({.vertexShader = m_shaders->get(EShaderId::kFullscreenVertex),
                                                            .fragmentShader = m_shaders->get(EShaderId::kPostFragment),
                                                            .colorTargetCount = 1,
                                                            .colorFormats = {format},
                                                            .debugName = "post"});
        m_pipelineFormat = format;
        if (!m_pipeline.valid()) {
            return;
        }
    }

    rhi::RenderPassDesc_t pass{.colorCount = 1, .debugName = "post"};
    pass.colors[0] = {.texture = context.backbuffer.backbuffer, .load = rhi::ELoadOp::kDontCare, .store = rhi::EStoreOp::kStore};
    rhi::IRhiCommandList& commands = context.commands;
    commands.beginPass(pass);
    commands.bindPipeline(m_pipeline);
    commands.setUniforms(rhi::EShaderStage::kFragment, gpu::kFrameUniformSlot, context.frame);
    commands.bindTexture(rhi::EShaderStage::kFragment, 0, context.targets.hdr(), context.resources.linearClampSampler());
    commands.draw(3, 1, 0);
    commands.endPass();
    ++context.stats.passes;
}

}  // namespace game
