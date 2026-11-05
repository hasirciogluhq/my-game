#include "engine/render/passes/shadow_pass.hpp"

#include "engine/render/vertex_layouts.hpp"

namespace game {

bool CShadowPass::create(const PassInitContext_t& context) {
    m_pipeline = context.device.createGraphicsPipeline({.vertexShader = context.shaders.get(EShaderId::kShadowVertex),
                                                        .fragmentShader = context.shaders.get(EShaderId::kShadowFragment),
                                                        .vertexLayout = rhiVertexLayout(EVertexLayout::kPosNormalUv),
                                                        .depth = {.test = true, .write = true, .compare = rhi::EDepthCompare::kLess},
                                                        .depthFormat = kDepthFormat,
                                                        .debugName = "shadow"});
    return m_pipeline.valid();
}

void CShadowPass::destroy(rhi::IRhiDevice& device) {
    device.destroy(m_pipeline);
    m_pipeline = {};
}

void CShadowPass::execute(const FrameContext_t& context) {
    rhi::IRhiCommandList& commands = context.commands;
    commands.beginPass({.depth = {.texture = context.targets.shadowMap(), .load = rhi::ELoadOp::kClear, .store = rhi::EStoreOp::kStore},
                        .debugName = "shadow"});
    if (context.shadowsEnabled) {
        commands.bindPipeline(m_pipeline);
        commands.setUniforms(rhi::EShaderStage::kVertex, gpu::kPassUniformSlot,
                             gpu::PassUniforms_t{.viewProjection = context.scene.lightViewProjection,
                                                 .lightViewProjection = context.scene.lightViewProjection});
        for (const SceneDraw_t& draw : context.scene.draws) {
            if (draw.castShadow) {
                passes::drawMesh(commands, *draw.mesh, draw.model);
                ++context.stats.shadowDrawCalls;
            }
        }
    }
    commands.endPass();
    ++context.stats.passes;
}

}  // namespace game
