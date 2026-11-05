#pragma once

#include "engine/interface/i_renderer.hpp"
#include "engine/render/api.hpp"
#include "engine/render/frame_context.hpp"
#include "engine/render/passes/render_pass.hpp"

#include <memory>
#include <string>
#include <vector>

namespace game {

// Renderer001 implementation. It is unaware of the backend: everything goes through rhi::IRhiDevice.
//
//   application -> CRenderSystem (resources, scene state, culling, sorting)
//               -> pass list: shadow -> scene (sky + opaque + transparent) -> debug -> post -> overlay
//               -> rhi::IRhiDevice / IRhiCommandList -> backend (CSdlGpuDevice = Metal; later D3D11 ...)
//
// New backend: add an IRhiDevice implementation to rhi_factory.cpp and write that language's source under shaders/.
// New effect: write a pass from IRenderPass and add it to createPasses().
class RENDER_API CRenderSystem final : public IRenderer {
public:
    CRenderSystem();
    CRenderSystem(const CRenderSystem&) = delete;
    CRenderSystem& operator=(const CRenderSystem&) = delete;
    ~CRenderSystem() override;

    [[nodiscard]] bool initialize(const RendererDesc_t& desc) override;
    void shutdown() override;
    [[nodiscard]] const char* lastError() const override { return m_lastError.c_str(); }
    [[nodiscard]] const RenderCaps_t& capabilities() const override { return m_caps; }
    void applySettings(const RenderSettings_t& settings) override;
    [[nodiscard]] const RenderSettings_t& settings() const override { return m_settings; }

    [[nodiscard]] MeshHandle_t createMesh(const MeshDesc_t& desc) override;
    void destroyMesh(MeshHandle_t mesh) override;
    [[nodiscard]] TextureHandle_t createTexture(const TextureDesc_t& desc) override;
    void destroyTexture(TextureHandle_t texture) override;
    [[nodiscard]] MaterialHandle_t createMaterial(const MaterialDesc_t& desc) override;
    void updateMaterial(MaterialHandle_t material, const MaterialDesc_t& desc) override;
    void destroyMaterial(MaterialHandle_t material) override;
    [[nodiscard]] MaterialHandle_t defaultMaterial() const override { return m_resources.defaultMaterial(); }

    void beginFrame(const FrameDesc_t& frame) override;
    void setView(const ViewDesc_t& view) override { m_scene.view = view; }
    void setEnvironment(const EnvironmentDesc_t& environment) override { m_scene.environment = environment; }
    void setPostProcess(const PostProcessDesc_t& post) override { m_scene.post = post; }
    void submit(const DrawItem_t& item) override;
    void submitLight(const LightDesc_t& light) override;
    void submitDebugLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) override;
    void setOverlay(RenderOverlayFn overlay, void* user) override;
    [[nodiscard]] void* nativeDevice() const override;
    void endFrame() override;

    [[nodiscard]] const RenderStats_t& stats() const override { return m_stats; }

private:
    bool fail(std::string message);
    [[nodiscard]] bool createPasses();
    void renderFrame(const rhi::RhiFrameInfo_t& backbuffer);
    void prepareScene(uint32_t shadowSize);
    void resolveDraws();

    std::unique_ptr<rhi::IRhiDevice> m_device;
    CShaderLibrary m_shaders;
    CRenderResources m_resources;
    CRenderTargets m_targets;
    std::vector<std::unique_ptr<IRenderPass>> m_passes;

    RenderSettings_t m_settings;
    RenderCaps_t m_caps;
    RenderStats_t m_stats;
    SceneState_t m_scene;
    std::string m_lastError;
    bool m_initialized = false;
};

}  // namespace game
