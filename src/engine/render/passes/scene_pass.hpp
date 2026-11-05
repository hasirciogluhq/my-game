#pragma once

#include "engine/render/passes/render_pass.hpp"

#include <unordered_map>

namespace game {

// HDR sahne pass'i: gökyüzü, opak mesh'ler (grup sıralı), şeffaf mesh'ler (arkadan öne).
// Pipeline'lar (shading × blend × cull) ilk kullanımda üretilip saklanır.
class CScenePass final : public IRenderPass {
public:
    [[nodiscard]] bool create(const PassInitContext_t& context) override;
    void destroy(rhi::IRhiDevice& device) override;
    void execute(const FrameContext_t& context) override;

    // Material'dan pipeline anahtarı (SceneDraw_t::pipelineKey).
    [[nodiscard]] static uint32_t pipelineKeyFor(const MaterialDesc_t& material);

private:
    [[nodiscard]] rhi::PipelineHandle_t pipelineFor(uint32_t key);
    void drawList(const FrameContext_t& context, const std::vector<uint32_t>& order);

    rhi::IRhiDevice* m_device = nullptr;
    CShaderLibrary* m_shaders = nullptr;
    rhi::PipelineHandle_t m_skyPipeline;
    std::unordered_map<uint32_t, rhi::PipelineHandle_t> m_pipelines;
};

}  // namespace game
