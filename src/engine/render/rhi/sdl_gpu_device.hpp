#pragma once

// SDL_GPU backend. SDL_GPU kendisi Metal/Vulkan/D3D12 üzerinde çalışır; bu sınıf hangisinin kullanılacağını
// SdlGpuConfig_t ile alır (şimdilik macOS: Metal + MSL). SDL bu dosyanın dışına sızmaz (struct ileri bildirimleri).
// D3D11 gibi SDL_GPU'nun desteklemediği API'ler için ayrı bir IRhiDevice yazılır (rhi/d3d11/...).

#include "engine/render/handle_pool.hpp"
#include "engine/render/rhi/rhi_device.hpp"

#include <array>
#include <string>

struct SDL_GPUBuffer;
struct SDL_GPUCommandBuffer;
struct SDL_GPUDevice;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPURenderPass;
struct SDL_GPUSampler;
struct SDL_GPUShader;
struct SDL_GPUTexture;
struct SDL_Window;

namespace game::rhi {

struct SdlGpuConfig_t {
    uint32_t shaderFormats = 0;        // SDL_GPUShaderFormat bayrakları
    const char* driverName = nullptr;  // "metal", "vulkan", "direct3d12" veya nullptr (SDL seçer)
    EShaderLanguage language = EShaderLanguage::kMsl;
    ERenderBackend backend = ERenderBackend::kMetal;
    const char* backendName = "SDL_GPU/Metal";
};

struct SdlBuffer_t {
    SDL_GPUBuffer* buffer = nullptr;
    uint32_t size = 0;
};

struct SdlTexture_t {
    SDL_GPUTexture* texture = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    EPixelFormat format = EPixelFormat::kUndefined;
    bool external = false;  // swapchain: sahibi biz değiliz
};

struct SdlSampler_t {
    SDL_GPUSampler* sampler = nullptr;
};

struct SdlShader_t {
    SDL_GPUShader* shader = nullptr;
    uint32_t uniformBufferCount = 0;
    EShaderStage stage = EShaderStage::kVertex;
};

struct SdlPipeline_t {
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    uint32_t vertexUniformCount = 0;
    uint32_t fragmentUniformCount = 0;
};

class CSdlGpuDevice;

class CSdlGpuCommandList final : public IRhiCommandList {
public:
    explicit CSdlGpuCommandList(CSdlGpuDevice& device) : m_device(device) {}

    // Device tarafından çağrılır.
    void begin(SDL_GPUCommandBuffer* commandBuffer);
    void end();

    void beginPass(const RenderPassDesc_t& pass) override;
    void endPass() override;
    void bindPipeline(PipelineHandle_t pipeline) override;
    void bindVertexBuffer(uint32_t slot, BufferHandle_t buffer, uint32_t offsetBytes) override;
    void bindIndexBuffer(BufferHandle_t buffer, EIndexType type, uint32_t offsetBytes) override;
    void setUniformBytes(EShaderStage stage, uint32_t slot, std::span<const std::byte> data) override;
    void bindTexture(EShaderStage stage, uint32_t slot, TextureHandle_t texture, SamplerHandle_t sampler) override;
    void setViewport(const Viewport_t& viewport) override;
    void setScissor(const ScissorRect_t& scissor) override;
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex) override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex) override;
    void recordNative(const RenderPassDesc_t& pass, RenderOverlayFn fn, void* user) override;
    void updateBuffer(BufferHandle_t buffer, uint32_t offsetBytes, std::span<const std::byte> data, EUpdateMode mode) override;

private:
    struct UniformSlot_t {
        std::array<std::byte, kMaxUniformBytes> data{};
        uint32_t size = 0;
        bool dirty = false;
    };
    struct TextureSlot_t {
        SDL_GPUTexture* texture = nullptr;
        SDL_GPUSampler* sampler = nullptr;
        bool dirty = false;
    };
    static constexpr size_t kStageCount = 2;

    [[nodiscard]] bool prepareDraw();
    void markAllDirty();

    CSdlGpuDevice& m_device;
    SDL_GPUCommandBuffer* m_commandBuffer = nullptr;
    SDL_GPURenderPass* m_pass = nullptr;
    bool m_pipelineBound = false;
    std::array<uint32_t, kStageCount> m_uniformCounts{};  // bağlı pipeline'ın stage başına uniform sayısı
    EIndexType m_indexType = EIndexType::kUint32;
    std::array<std::array<UniformSlot_t, kMaxUniformSlots>, kStageCount> m_uniforms;
    std::array<std::array<TextureSlot_t, kMaxTextureSlots>, kStageCount> m_textures;
};

class CSdlGpuDevice final : public IRhiDevice {
public:
    explicit CSdlGpuDevice(SdlGpuConfig_t config);
    CSdlGpuDevice(const CSdlGpuDevice&) = delete;
    CSdlGpuDevice& operator=(const CSdlGpuDevice&) = delete;
    ~CSdlGpuDevice() override;

    [[nodiscard]] bool initialize(const RhiDeviceDesc_t& desc) override;
    void shutdown() override;
    [[nodiscard]] const RhiCaps_t& caps() const override { return m_caps; }
    [[nodiscard]] const std::string& lastError() const override { return m_lastError; }

    [[nodiscard]] BufferHandle_t createBuffer(const BufferDesc_t& desc, std::span<const std::byte> initialData) override;
    [[nodiscard]] TextureHandle_t createTexture(const TextureDesc_t& desc, std::span<const std::byte> initialPixels) override;
    [[nodiscard]] SamplerHandle_t createSampler(const SamplerDesc_t& desc) override;
    [[nodiscard]] ShaderHandle_t createShader(const ShaderDesc_t& desc) override;
    [[nodiscard]] PipelineHandle_t createGraphicsPipeline(const GraphicsPipelineDesc_t& desc) override;

    void destroy(BufferHandle_t handle) override;
    void destroy(TextureHandle_t handle) override;
    void destroy(SamplerHandle_t handle) override;
    void destroy(ShaderHandle_t handle) override;
    void destroy(PipelineHandle_t handle) override;

    void setVsync(bool enabled) override;
    [[nodiscard]] void* nativeDevice() const override { return m_device; }

    [[nodiscard]] std::optional<RhiFrameInfo_t> beginFrame() override;
    [[nodiscard]] IRhiCommandList& commands() override { return m_commands; }
    void endFrame() override;
    void waitIdle() override;

    // Command list'in kullandığı iç erişim.
    [[nodiscard]] SDL_GPUDevice* sdlDevice() const { return m_device; }
    [[nodiscard]] const SdlBuffer_t* find(BufferHandle_t handle) const { return m_buffers.get(handle); }
    [[nodiscard]] const SdlTexture_t* find(TextureHandle_t handle) const { return m_textures.get(handle); }
    [[nodiscard]] const SdlSampler_t* find(SamplerHandle_t handle) const { return m_samplers.get(handle); }
    [[nodiscard]] const SdlShader_t* find(ShaderHandle_t handle) const { return m_shaders.get(handle); }
    [[nodiscard]] const SdlPipeline_t* find(PipelineHandle_t handle) const { return m_pipelines.get(handle); }

    // Hata mesajı kaydeder ve stderr'e yazar.
    void fail(const std::string& message);

private:
    [[nodiscard]] bool applyPresentMode(bool vsync);

    SdlGpuConfig_t m_config;
    RhiCaps_t m_caps;
    std::string m_lastError;

    SDL_Window* m_window = nullptr;
    SDL_GPUDevice* m_device = nullptr;
    EPixelFormat m_swapchainFormat = EPixelFormat::kUndefined;
    TextureHandle_t m_swapchainHandle;
    SDL_GPUCommandBuffer* m_frameCommands = nullptr;
    bool m_frameActive = false;

    CHandlePool<BufferHandle_t, SdlBuffer_t> m_buffers;
    CHandlePool<TextureHandle_t, SdlTexture_t> m_textures;
    CHandlePool<SamplerHandle_t, SdlSampler_t> m_samplers;
    CHandlePool<ShaderHandle_t, SdlShader_t> m_shaders;
    CHandlePool<PipelineHandle_t, SdlPipeline_t> m_pipelines;

    CSdlGpuCommandList m_commands{*this};
};

}  // namespace game::rhi
