#pragma once

#include "engine/render/rhi/rhi_types.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <type_traits>

namespace game::rhi {

struct RhiDeviceDesc_t {
    RenderSurface_t surface;
    bool vsync = true;
    bool debugLayer = false;
};

struct RhiCaps_t {
    ERenderBackend backend = ERenderBackend::kAuto;
    const char* backendName = "";
    EShaderLanguage shaderLanguage = EShaderLanguage::kMsl;
    uint32_t maxTextureSize = 0;
    bool depthZeroToOne = true;
};

// The screen target returned by beginFrame, belonging to this frame. backbuffer is valid only until endFrame.
struct RhiFrameInfo_t {
    TextureHandle_t backbuffer;
    uint32_t width = 0;
    uint32_t height = 0;
    EPixelFormat format = EPixelFormat::kUndefined;
};

// Command recording. State (pipeline, uniforms, textures) persists within a pass; when the pipeline changes, the backend rebinds if needed.
// Draw commands are called between beginPass/endPass, while updateBuffer is called outside a pass.
class IRhiCommandList {
public:
    virtual ~IRhiCommandList() = default;

    virtual void beginPass(const RenderPassDesc_t& pass) = 0;
    virtual void endPass() = 0;

    virtual void bindPipeline(PipelineHandle_t pipeline) = 0;
    virtual void bindVertexBuffer(uint32_t slot, BufferHandle_t buffer, uint32_t offsetBytes) = 0;
    virtual void bindIndexBuffer(BufferHandle_t buffer, EIndexType type, uint32_t offsetBytes) = 0;
    virtual void setUniformBytes(EShaderStage stage, uint32_t slot, std::span<const std::byte> data) = 0;
    virtual void bindTexture(EShaderStage stage, uint32_t slot, TextureHandle_t texture, SamplerHandle_t sampler) = 0;
    virtual void setViewport(const Viewport_t& viewport) = 0;
    virtual void setScissor(const ScissorRect_t& scissor) = 0;

    virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex) = 0;
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex) = 0;

    // Recording into the backend's own command buffer from outside (Dear ImGui backend). Outside a pass:
    // fn(user, cmd, nullptr); then `pass` is opened, fn(user, cmd, nativePass); then the pass is closed.
    virtual void recordNative(const RenderPassDesc_t& pass, RenderOverlayFn fn, void* user) = 0;

    // Outside a pass. The data is ordered into this frame's command stream.
    virtual void updateBuffer(BufferHandle_t buffer, uint32_t offsetBytes, std::span<const std::byte> data, EUpdateMode mode) = 0;

    template <typename T>
    void setUniforms(EShaderStage stage, uint32_t slot, const T& value) {
        static_assert(std::is_trivially_copyable_v<T>);
        setUniformBytes(stage, slot, std::as_bytes(std::span{&value, 1}));
    }
};

class IRhiDevice {
public:
    virtual ~IRhiDevice() = default;

    [[nodiscard]] virtual bool initialize(const RhiDeviceDesc_t& desc) = 0;
    virtual void shutdown() = 0;
    [[nodiscard]] virtual const RhiCaps_t& caps() const = 0;
    [[nodiscard]] virtual const std::string& lastError() const = 0;

    // --- resources. initialData may be empty. On error: an invalid handle ---
    [[nodiscard]] virtual BufferHandle_t createBuffer(const BufferDesc_t& desc, std::span<const std::byte> initialData) = 0;
    [[nodiscard]] virtual TextureHandle_t createTexture(const TextureDesc_t& desc, std::span<const std::byte> initialPixels) = 0;
    [[nodiscard]] virtual SamplerHandle_t createSampler(const SamplerDesc_t& desc) = 0;
    [[nodiscard]] virtual ShaderHandle_t createShader(const ShaderDesc_t& desc) = 0;
    [[nodiscard]] virtual PipelineHandle_t createGraphicsPipeline(const GraphicsPipelineDesc_t& desc) = 0;

    // Safe: if the GPU is still using the resource, the backend defers destruction.
    virtual void destroy(BufferHandle_t handle) = 0;
    virtual void destroy(TextureHandle_t handle) = 0;
    virtual void destroy(SamplerHandle_t handle) = 0;
    virtual void destroy(ShaderHandle_t handle) = 0;
    virtual void destroy(PipelineHandle_t handle) = 0;

    virtual void setVsync(bool enabled) = 0;
    // The backend's native device (SDL_GPU: SDL_GPUDevice*). For external library backends.
    [[nodiscard]] virtual void* nativeDevice() const = 0;

    // Frame: if the swapchain cannot be acquired (window too small), nullopt; that frame is skipped and endFrame is not called.
    [[nodiscard]] virtual std::optional<RhiFrameInfo_t> beginFrame() = 0;
    [[nodiscard]] virtual IRhiCommandList& commands() = 0;
    virtual void endFrame() = 0;  // submits + presents

    virtual void waitIdle() = 0;
};

// Backend selection: render/rhi/rhi_factory.cpp
[[nodiscard]] std::unique_ptr<IRhiDevice> createRhiDevice(ERenderBackend backend, std::string& error);

}  // namespace game::rhi
