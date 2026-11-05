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
    m_skinnedPipeline = context.device.createGraphicsPipeline({.vertexShader = context.shaders.get(EShaderId::kShadowSkinnedVertex),
                                                               .fragmentShader = context.shaders.get(EShaderId::kShadowFragment),
                                                               .vertexLayout = rhiVertexLayout(EVertexLayout::kPosNormalUvSkin),
                                                               .depth = {.test = true, .write = true, .compare = rhi::EDepthCompare::kLess},
                                                               .depthFormat = kDepthFormat,
                                                               .debugName = "shadow_skinned"});
    return m_pipeline.valid() && m_skinnedPipeline.valid();
}

void CShadowPass::destroy(rhi::IRhiDevice& device) {
    device.destroy(m_pipeline);
    device.destroy(m_skinnedPipeline);
    m_pipeline = {};
    m_skinnedPipeline = {};
}

void CShadowPass::execute(const FrameContext_t& context) {
    rhi::IRhiCommandList& commands = context.commands;
    commands.beginPass({.depth = {.texture = context.targets.shadowMap(), .load = rhi::ELoadOp::kClear, .store = rhi::EStoreOp::kStore},
                        .debugName = "shadow"});
    if (context.shadowsEnabled) {
        commands.setUniforms(rhi::EShaderStage::kVertex, gpu::kPassUniformSlot,
                             gpu::PassUniforms_t{.viewProjection = context.scene.lightViewProjection,
                                                 .lightViewProjection = context.scene.lightViewProjection});
        bool skinnedBound = !context.scene.draws.empty() && context.scene.draws.front().skinned;
        commands.bindPipeline(skinnedBound ? m_skinnedPipeline : m_pipeline);
        for (const SceneDraw_t& draw : context.scene.draws) {
            if (!draw.castShadow) {
                continue;
            }
            if (draw.skinned != skinnedBound) {
                skinnedBound = draw.skinned;
                commands.bindPipeline(skinnedBound ? m_skinnedPipeline : m_pipeline);
            }
            if (draw.skinned) {
                passes::setBones(commands, context.scene, draw);
            }
            passes::drawMesh(commands, *draw.mesh, draw.model);
            ++context.stats.shadowDrawCalls;
        }
    }
    commands.endPass();
    ++context.stats.passes;
}

}  // namespace game
