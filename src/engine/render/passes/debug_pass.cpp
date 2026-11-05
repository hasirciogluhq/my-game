#include "engine/render/passes/debug_pass.hpp"

#include "engine/render/vertex_layouts.hpp"

#include <algorithm>
#include <span>
#include <utility>

namespace game {
namespace {

constexpr uint32_t kInitialVertices = 4096;

}  // namespace

bool CDebugPass::create(const PassInitContext_t& context) {
    m_pipeline = context.device.createGraphicsPipeline({.vertexShader = context.shaders.get(EShaderId::kDebugVertex),
                                                        .fragmentShader = context.shaders.get(EShaderId::kDebugFragment),
                                                        .vertexLayout = debugVertexLayout(),
                                                        .topology = rhi::EPrimitiveTopology::kLineList,
                                                        .depth = {.test = true, .write = false, .compare = rhi::EDepthCompare::kLess},
                                                        .colorTargetCount = 1,
                                                        .colorFormats = {kHdrFormat},
                                                        .depthFormat = kDepthFormat,
                                                        .debugName = "debug"});
    return m_pipeline.valid();
}

void CDebugPass::destroy(rhi::IRhiDevice& device) {
    device.destroy(std::exchange(m_pipeline, {}));
    device.destroy(std::exchange(m_buffer, {}));
    m_capacityVertices = 0;
}

bool CDebugPass::ensureCapacity(rhi::IRhiDevice& device, uint32_t vertexCount) {
    if (m_buffer.valid() && m_capacityVertices >= vertexCount) {
        return true;
    }
    uint32_t capacity = std::max(m_capacityVertices, kInitialVertices);
    while (capacity < vertexCount) {
        capacity *= 2;
    }
    device.destroy(std::exchange(m_buffer, {}));
    m_buffer = device.createBuffer({.sizeBytes = capacity * static_cast<uint32_t>(sizeof(gpu::DebugVertex_t)),
                                    .usage = rhi::EBufferUsage::kVertex,
                                    .debugName = "debug-lines"}, {});
    m_capacityVertices = m_buffer.valid() ? capacity : 0;
    return m_buffer.valid();
}

void CDebugPass::execute(const FrameContext_t& context) {
    const std::vector<gpu::DebugVertex_t>& vertices = context.scene.debugVertices;
    if (vertices.empty() || !ensureCapacity(context.device, static_cast<uint32_t>(vertices.size()))) {
        return;
    }
    rhi::IRhiCommandList& commands = context.commands;
    commands.updateBuffer(m_buffer, 0, std::as_bytes(std::span{vertices}), rhi::EUpdateMode::kDiscard);

    rhi::RenderPassDesc_t pass{.colorCount = 1,
                               .depth = {.texture = context.targets.depth(), .load = rhi::ELoadOp::kLoad, .store = rhi::EStoreOp::kDontCare},
                               .debugName = "debug"};
    pass.colors[0] = {.texture = context.targets.hdr(), .load = rhi::ELoadOp::kLoad, .store = rhi::EStoreOp::kStore};
    commands.beginPass(pass);
    commands.bindPipeline(m_pipeline);
    commands.setUniforms(rhi::EShaderStage::kVertex, gpu::kPassUniformSlot,
                         gpu::PassUniforms_t{.viewProjection = context.scene.viewProjection,
                                             .lightViewProjection = context.scene.viewProjection});
    commands.bindVertexBuffer(0, m_buffer, 0);
    commands.draw(static_cast<uint32_t>(vertices.size()), 1, 0);
    commands.endPass();
    ++context.stats.passes;
}

}  // namespace game
