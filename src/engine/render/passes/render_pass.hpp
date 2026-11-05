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

// Frame'i oluşturan sıralı adımlar. CRenderSystem pass'leri sırayla çalıştırır:
// shadow -> scene (gökyüzü + opak + şeffaf) -> debug -> post. Yeni efekt = yeni pass sınıfı + listeye ekleme.
class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    [[nodiscard]] virtual bool create(const PassInitContext_t& context) = 0;
    virtual void destroy(rhi::IRhiDevice& device) = 0;
    virtual void execute(const FrameContext_t& context) = 0;
};

namespace passes {

// İskeletli çizimin skin matrislerini bone uniform bloğuna yazar (eksik eklemler birim matris = bind pozu).
inline void setBones(rhi::IRhiCommandList& commands, const SceneState_t& scene, const SceneDraw_t& draw) {
    gpu::BoneUniforms_t uniforms;
    std::ranges::fill(uniforms.bones, glm::mat4{1.0f});
    const auto first = scene.boneMatrices.begin() + draw.boneOffset;
    std::copy(first, first + draw.boneCount, std::begin(uniforms.bones));
    commands.setUniforms(rhi::EShaderStage::kVertex, gpu::kBoneUniformSlot, uniforms);
}

// Mesh'i bağlayıp çizer (pipeline ve ortak uniform'lar çağıranda hazır olmalı).
inline void drawMesh(rhi::IRhiCommandList& commands, const GpuMesh_t& mesh, const glm::mat4& model) {
    commands.bindVertexBuffer(0, mesh.vertexBuffer, 0);
    commands.bindIndexBuffer(mesh.indexBuffer, mesh.indexType, 0);
    commands.setUniforms(rhi::EShaderStage::kVertex, gpu::kDrawUniformSlot, gpu::DrawUniforms_t{.model = model});
    commands.drawIndexed(mesh.indexCount, 1, 0, 0);
}

}  // namespace passes
}  // namespace game
