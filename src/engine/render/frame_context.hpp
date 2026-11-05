#pragma once

#include "engine/interface/render_types.hpp"
#include "engine/render/gpu_structs.hpp"
#include "engine/render/render_resources.hpp"
#include "engine/render/render_targets.hpp"
#include "engine/render/rhi/rhi_device.hpp"
#include "engine/render/shader_library.hpp"

#include <vector>

namespace game {

// A single draw resolved (handle -> pointer) in endFrame.
struct SceneDraw_t {
    const GpuMesh_t* mesh = nullptr;
    const MaterialDesc_t* material = nullptr;
    rhi::TextureHandle_t baseTexture;
    glm::mat4 model{1.0f};
    uint64_t sortKey = 0;        // opaque: grouping by pipeline > material > mesh
    float distanceSquared = 0.0f;  // squared camera distance (transparent sorting)
    uint32_t pipelineKey = 0;
    bool castShadow = false;
    bool skinned = false;          // kPosNormalUvSkin: skin matrices are SceneState_t::boneMatrices[boneOffset, +boneCount)
    uint32_t boneOffset = 0;
    uint32_t boneCount = 0;
};

// A draw coming in through submit(). Skin matrices do not stay in the caller's span: they are copied into SceneState_t::boneMatrices.
struct SubmittedDraw_t {
    DrawItem_t item;
    uint32_t boneOffset = 0;
    uint32_t boneCount = 0;
};

// Persist across calls: view, environment, post, overlay. Cleared every frame: submitted, lights, debug.
struct SceneState_t {
    FrameDesc_t frame;
    ViewDesc_t view;
    EnvironmentDesc_t environment;
    PostProcessDesc_t post;

    std::vector<SubmittedDraw_t> submitted;
    std::vector<glm::mat4> boneMatrices;  // per frame, in submit order
    std::vector<LightDesc_t> lights;
    std::vector<gpu::DebugVertex_t> debugVertices;
    RenderOverlayFn overlay = nullptr;  // setOverlay: persistent, called every frame as the last step onto the swapchain
    void* overlayUser = nullptr;

    // Computed during endFrame.
    glm::mat4 viewProjection{1.0f};
    glm::mat4 lightViewProjection{1.0f};
    glm::vec3 cameraPosition{0.0f};
    std::vector<SceneDraw_t> draws;     // all visible + shadow-casting draws
    std::vector<uint32_t> opaque;       // draws indices, grouped and sorted
    std::vector<uint32_t> blended;      // draws indices, back to front
};

// Shared input of the passes. Does not change during the frame (frame uniforms are computed at the start of endFrame).
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
