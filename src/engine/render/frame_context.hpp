#pragma once

#include "engine/interface/render_types.hpp"
#include "engine/render/gpu_structs.hpp"
#include "engine/render/render_resources.hpp"
#include "engine/render/render_targets.hpp"
#include "engine/render/rhi/rhi_device.hpp"
#include "engine/render/shader_library.hpp"

#include <vector>

namespace game {

// endFrame'de çözülmüş (handle -> pointer) tek çizim.
struct SceneDraw_t {
    const GpuMesh_t* mesh = nullptr;
    const MaterialDesc_t* material = nullptr;
    rhi::TextureHandle_t baseTexture;
    glm::mat4 model{1.0f};
    uint64_t sortKey = 0;        // opak: pipeline > material > mesh gruplaması
    float distanceSquared = 0.0f;  // kamera mesafesi² (şeffaf sıralaması)
    uint32_t pipelineKey = 0;
    bool castShadow = false;
};

// Çağrılar arasında kalıcı: view, environment, post, overlay. Frame başına temizlenen: submitted, lights, debug.
struct SceneState_t {
    FrameDesc_t frame;
    ViewDesc_t view;
    EnvironmentDesc_t environment;
    PostProcessDesc_t post;

    std::vector<DrawItem_t> submitted;
    std::vector<LightDesc_t> lights;
    std::vector<gpu::DebugVertex_t> debugVertices;
    RenderOverlayFn overlay = nullptr;  // setOverlay: kalıcı, her frame swapchain'e son adım olarak çağrılır
    void* overlayUser = nullptr;

    // endFrame sırasında hesaplanır.
    glm::mat4 viewProjection{1.0f};
    glm::mat4 lightViewProjection{1.0f};
    glm::vec3 cameraPosition{0.0f};
    std::vector<SceneDraw_t> draws;     // görünür + gölge düşüren tüm çizimler
    std::vector<uint32_t> opaque;       // draws indeksleri, grup sıralı
    std::vector<uint32_t> blended;      // draws indeksleri, arkadan öne
};

// Pass'lerin ortak girdisi. Frame boyunca değişmez (frame uniform'ları endFrame başında hesaplanır).
struct FrameContext_t {
    rhi::IRhiDevice& device;
    rhi::IRhiCommandList& commands;
    CShaderLibrary& shaders;
    CRenderResources& resources;
    CRenderTargets& targets;
    const rhi::RhiFrameInfo_t& backbuffer;
    const RenderSettings_t& settings;
    const SceneState_t& scene;
    const gpu::FrameUniforms_t& frame;
    RenderStats_t& stats;
    bool shadowsEnabled = false;
};

}  // namespace game
