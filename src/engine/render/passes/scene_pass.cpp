#include "engine/render/passes/scene_pass.hpp"

#include "engine/render/vertex_layouts.hpp"

namespace game {
namespace {

// pipeline anahtarı: bit 0-1 shading, bit 2 blend, bit 3 doubleSided, bit 4 iskeletli mesh
constexpr uint32_t kBlendBit = 1u << 2;
constexpr uint32_t kDoubleSidedBit = 1u << 3;
constexpr uint32_t kSkinnedBit = 1u << 4;

EShaderId fragmentShaderFor(EShadingModel shading) {
    switch (shading) {
        case EShadingModel::kLit: return EShaderId::kLitFragment;
        case EShadingModel::kRimLit: return EShaderId::kRimLitFragment;
        case EShadingModel::kUnlit: return EShaderId::kUnlitFragment;
    }
    return EShaderId::kLitFragment;
}

gpu::MaterialUniforms_t makeMaterialUniforms(const MaterialDesc_t& material) {
    return {.baseColor = material.baseColor,
            .emissiveGrid = glm::vec4{material.emissive, material.gridStrength},
            .params = {material.specularPower, material.specularAmount, material.rimAmount, 0.0f}};
}

}  // namespace

uint32_t CScenePass::pipelineKeyFor(const MaterialDesc_t& material, bool skinned) {
    return static_cast<uint32_t>(material.shading) | (material.blend == EBlendMode::kAlphaBlend ? kBlendBit : 0u) |
           (material.doubleSided ? kDoubleSidedBit : 0u) | (skinned ? kSkinnedBit : 0u);
}

bool CScenePass::create(const PassInitContext_t& context) {
    m_device = &context.device;
    m_shaders = &context.shaders;
    m_skyPipeline = context.device.createGraphicsPipeline({.vertexShader = context.shaders.get(EShaderId::kFullscreenVertex),
                                                           .fragmentShader = context.shaders.get(EShaderId::kSkyFragment),
                                                           .colorTargetCount = 1,
                                                           .colorFormats = {kHdrFormat},
                                                           .depthFormat = kDepthFormat,
                                                           .debugName = "sky"});
    return m_skyPipeline.valid();
}

void CScenePass::destroy(rhi::IRhiDevice& device) {
    device.destroy(m_skyPipeline);
    m_skyPipeline = {};
    for (auto& [key, pipeline] : m_pipelines) {
        device.destroy(pipeline);
    }
    m_pipelines.clear();
}

rhi::PipelineHandle_t CScenePass::pipelineFor(uint32_t key) {
    if (const auto found = m_pipelines.find(key); found != m_pipelines.end()) {
        return found->second;
    }
    const bool blended = (key & kBlendBit) != 0;
    const bool skinned = (key & kSkinnedBit) != 0;
    const auto shading = static_cast<EShadingModel>(key & 3u);
    const rhi::PipelineHandle_t pipeline = m_device->createGraphicsPipeline(
        {.vertexShader = m_shaders->get(skinned ? EShaderId::kSceneSkinnedVertex : EShaderId::kSceneVertex),
         .fragmentShader = m_shaders->get(fragmentShaderFor(shading)),
         .vertexLayout = rhiVertexLayout(skinned ? EVertexLayout::kPosNormalUvSkin : EVertexLayout::kPosNormalUv),
         .cull = (key & kDoubleSidedBit) != 0 ? rhi::ECullMode::kNone : rhi::ECullMode::kBack,
         .depth = {.test = true, .write = !blended, .compare = rhi::EDepthCompare::kLess},
         .blend = blended ? rhi::EBlendPreset::kAlpha : rhi::EBlendPreset::kNone,
         .colorTargetCount = 1,
         .colorFormats = {kHdrFormat},
         .depthFormat = kDepthFormat,
         .debugName = "scene"});
    m_pipelines.emplace(key, pipeline);  // başarısız (geçersiz) handle da saklanır: her frame yeniden denenmez
    return pipeline;
}

void CScenePass::drawList(const FrameContext_t& context, const std::vector<uint32_t>& order) {
    rhi::IRhiCommandList& commands = context.commands;
    uint32_t lastKey = ~0u;
    bool pipelineOk = false;
    const MaterialDesc_t* lastMaterial = nullptr;
    rhi::TextureHandle_t lastTexture;

    for (const uint32_t index : order) {
        const SceneDraw_t& draw = context.scene.draws[index];
        if (draw.pipelineKey != lastKey) {
            const rhi::PipelineHandle_t pipeline = pipelineFor(draw.pipelineKey);
            pipelineOk = pipeline.valid();
            lastKey = draw.pipelineKey;
            if (pipelineOk) {
                commands.bindPipeline(pipeline);
            }
        }
        if (!pipelineOk) {
            continue;
        }
        if (draw.material != lastMaterial) {
            commands.setUniforms(rhi::EShaderStage::kFragment, gpu::kMaterialUniformSlot, makeMaterialUniforms(*draw.material));
            lastMaterial = draw.material;
        }
        if (draw.baseTexture != lastTexture) {
            commands.bindTexture(rhi::EShaderStage::kFragment, 1, draw.baseTexture, context.resources.materialSampler());
            lastTexture = draw.baseTexture;
        }
        if (draw.skinned) {
            passes::setBones(commands, context.scene, draw);
        }
        passes::drawMesh(commands, *draw.mesh, draw.model);
        ++context.stats.drawCalls;
        context.stats.triangles += draw.mesh->indexCount / 3;
    }
}

void CScenePass::execute(const FrameContext_t& context) {
    rhi::IRhiCommandList& commands = context.commands;
    const bool keepDepth = !context.scene.debugVertices.empty();  // debug pass derinliği okur

    rhi::RenderPassDesc_t pass{.colorCount = 1,
                               .depth = {.texture = context.targets.depth(),
                                         .load = rhi::ELoadOp::kClear,
                                         .store = keepDepth ? rhi::EStoreOp::kStore : rhi::EStoreOp::kDontCare},
                               .debugName = "scene"};
    pass.colors[0] = {.texture = context.targets.hdr(), .load = rhi::ELoadOp::kClear, .store = rhi::EStoreOp::kStore};
    commands.beginPass(pass);

    commands.setUniforms(rhi::EShaderStage::kFragment, gpu::kFrameUniformSlot, context.frame);

    commands.bindPipeline(m_skyPipeline);
    commands.draw(3, 1, 0);

    commands.setUniforms(rhi::EShaderStage::kVertex, gpu::kPassUniformSlot,
                         gpu::PassUniforms_t{.viewProjection = context.scene.viewProjection,
                                             .lightViewProjection = context.scene.lightViewProjection});
    commands.bindTexture(rhi::EShaderStage::kFragment, 0, context.targets.shadowMap(), context.resources.shadowSampler());
    drawList(context, context.scene.opaque);
    drawList(context, context.scene.blended);

    commands.endPass();
    ++context.stats.passes;
}

}  // namespace game
