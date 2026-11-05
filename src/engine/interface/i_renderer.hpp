#pragma once

#include "engine/interface/render_types.hpp"

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace game {

// ---------------------------------------------------------------------------
// Renderer001: backend-independent renderer (the single renderer interface). Types are in interface/render_types.hpp.
//
// Order of use:
//   initialize -> createMesh/createTexture/createMaterial (at load time; can also happen later)
//   -> every frame: beginFrame -> setView -> setEnvironment/setPostProcess -> submit* / submitLight* -> endFrame
//   -> shutdown
// Called from a single thread. Draws/lights/debug lines are recorded until endFrame; endFrame draws all of them.
// Invalid or destroyed handles are silently ignored (not drawn in submit).
// ---------------------------------------------------------------------------
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // --- lifecycle ---
    [[nodiscard]] virtual bool initialize(const RendererDesc_t& desc) = 0;
    virtual void shutdown() = 0;
    [[nodiscard]] virtual const char* lastError() const = 0;
    [[nodiscard]] virtual const RenderCaps_t& capabilities() const = 0;

    // Can change at runtime: vsync, shadow quality/range, render scale.
    virtual void applySettings(const RenderSettings_t& settings) = 0;
    [[nodiscard]] virtual const RenderSettings_t& settings() const = 0;

    // --- resources (an invalid handle return = error, see lastError()) ---
    [[nodiscard]] virtual MeshHandle_t createMesh(const MeshDesc_t& desc) = 0;
    virtual void destroyMesh(MeshHandle_t mesh) = 0;

    [[nodiscard]] virtual TextureHandle_t createTexture(const TextureDesc_t& desc) = 0;
    virtual void destroyTexture(TextureHandle_t texture) = 0;

    [[nodiscard]] virtual MaterialHandle_t createMaterial(const MaterialDesc_t& desc) = 0;
    virtual void updateMaterial(MaterialHandle_t material, const MaterialDesc_t& desc) = 0;
    virtual void destroyMaterial(MaterialHandle_t material) = 0;
    // White, kLit, opaque. Cannot be deleted.
    [[nodiscard]] virtual MaterialHandle_t defaultMaterial() const = 0;

    // --- frame ---
    virtual void beginFrame(const FrameDesc_t& frame) = 0;
    virtual void setView(const ViewDesc_t& view) = 0;
    virtual void setEnvironment(const EnvironmentDesc_t& environment) = 0;
    virtual void setPostProcess(const PostProcessDesc_t& post) = 0;

    virtual void submit(const DrawItem_t& item) = 0;
    virtual void submitLight(const LightDesc_t& light) = 0;
    // Depth-tested world-space line (drawn over the HDR scene). color is a raw sRGB-like value.
    virtual void submitDebugLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) = 0;

    // Overlay (Dear ImGui): the last step drawn onto the swapchain every frame after post-processing. Persistent; nullptr removes it.
    // The renderer does not know what is drawn; it hands over the backend's command buffer (see RenderOverlayFn).
    virtual void setOverlay(RenderOverlayFn overlay, void* user) = 0;
    // The backend's native device (SDL_GPU: SDL_GPUDevice*); nullptr before initialize. For external backends (ImGui).
    [[nodiscard]] virtual void* nativeDevice() const = 0;

    virtual void endFrame() = 0;

    [[nodiscard]] virtual const RenderStats_t& stats() const = 0;
};

inline constexpr char kRendererInterface[] = "Renderer001";

}  // namespace game
