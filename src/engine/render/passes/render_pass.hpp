#pragma once

#include "engine/render/frame_context.hpp"

#include <algorithm>
#include <iterator>

namespace game {

struct PassInitContext_t {
    rhi::IRhiDevice& device;
    CShaderLibrary& shaders;
    CRenderResources& resources;
};

// Sequential steps that build the frame. CRenderSystem runs the passes in order:
// shadow -> scene (sky + opaque + transparent) -> debug -> post. New effect = new pass class + add it to the list.
class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    [[nodiscard]] virtual bool create(const PassInitContext_t& context) = 0;
    virtual void destroy(rhi::IRhiDevice& device) = 0;
    virtual void execute(const FrameContext_t& context) = 0;
};

namespace passes {

// Writes the skin matrices of a skinned draw into the bone uniform block (missing joints are identity = bind pose).
inline void setBones(rhi::IRhiCommandList& commands, const SceneState_t& scene, const SceneDraw_t& draw) {
    gpu::BoneUniforms_t uniforms;
    std::ranges::fill(uniforms.bones, glm::mat4{1.0f});
    const auto first = scene.boneMatrices.begin() + draw.boneOffset;
    std::copy(first, first + draw.boneCount, std::begin(uniforms.bones));
    commands.setUniforms(rhi::EShaderStage::kVertex, gpu::kBoneUniformSlot, uniforms);
}

// Binds the mesh and draws it (the pipeline and shared uniforms must be ready in the caller).
inline void drawMesh(rhi::IRhiCommandList& commands, const GpuMesh_t& mesh, const glm::mat4& model) {
    commands.bindVertexBuffer(0, mesh.vertexBuffer, 0);
    commands.bindIndexBuffer(mesh.indexBuffer, mesh.indexType, 0);
    commands.setUniforms(rhi::EShaderStage::kVertex, gpu::kDrawUniformSlot, gpu::DrawUniforms_t{.model = model});
    commands.drawIndexed(mesh.indexCount, 1, 0, 0);
}

}  // namespace passes
}  // namespace game
