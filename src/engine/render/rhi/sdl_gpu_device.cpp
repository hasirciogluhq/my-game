#include "engine/render/rhi/sdl_gpu_device.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <utility>

namespace game::rhi {
namespace {

SDL_GPUTextureFormat toSdl(EPixelFormat format) {
    switch (format) {
        case EPixelFormat::kR8Unorm: return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
        case EPixelFormat::kRgba8Unorm: return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        case EPixelFormat::kRgba8UnormSrgb: return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
        case EPixelFormat::kBgra8Unorm: return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        case EPixelFormat::kBgra8UnormSrgb: return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB;
        case EPixelFormat::kRgba16Float: return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        case EPixelFormat::kD32Float: return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        case EPixelFormat::kUndefined: break;
    }
    return SDL_GPU_TEXTUREFORMAT_INVALID;
}

EPixelFormat fromSdl(SDL_GPUTextureFormat format) {
    switch (format) {
        case SDL_GPU_TEXTUREFORMAT_R8_UNORM: return EPixelFormat::kR8Unorm;
        case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM: return EPixelFormat::kRgba8Unorm;
        case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB: return EPixelFormat::kRgba8UnormSrgb;
        case SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM: return EPixelFormat::kBgra8Unorm;
        case SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB: return EPixelFormat::kBgra8UnormSrgb;
        case SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT: return EPixelFormat::kRgba16Float;
        case SDL_GPU_TEXTUREFORMAT_D32_FLOAT: return EPixelFormat::kD32Float;
        default: return EPixelFormat::kUndefined;
    }
}

SDL_GPUFilter toSdl(EFilter filter) {
    return filter == EFilter::kNearest ? SDL_GPU_FILTER_NEAREST : SDL_GPU_FILTER_LINEAR;
}

SDL_GPUSamplerMipmapMode toSdlMip(EFilter filter) {
    return filter == EFilter::kNearest ? SDL_GPU_SAMPLERMIPMAPMODE_NEAREST : SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
}

SDL_GPUSamplerAddressMode toSdl(EAddressMode mode) {
    switch (mode) {
        case EAddressMode::kRepeat: return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        case EAddressMode::kClamp: return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        case EAddressMode::kMirror: return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
    }
    return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
}

SDL_GPUVertexElementFormat toSdl(EVertexFormat format) {
    switch (format) {
        case EVertexFormat::kFloat2: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        case EVertexFormat::kFloat3: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        case EVertexFormat::kFloat4: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        case EVertexFormat::kUByte4Norm: return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    }
    return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
}

SDL_GPUCompareOp toSdl(EDepthCompare compare) {
    switch (compare) {
        case EDepthCompare::kNever: return SDL_GPU_COMPAREOP_NEVER;
        case EDepthCompare::kLess: return SDL_GPU_COMPAREOP_LESS;
        case EDepthCompare::kLessEqual: return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        case EDepthCompare::kAlways: return SDL_GPU_COMPAREOP_ALWAYS;
    }
    return SDL_GPU_COMPAREOP_LESS;
}

SDL_GPUCullMode toSdl(ECullMode cull) {
    switch (cull) {
        case ECullMode::kNone: return SDL_GPU_CULLMODE_NONE;
        case ECullMode::kBack: return SDL_GPU_CULLMODE_BACK;
        case ECullMode::kFront: return SDL_GPU_CULLMODE_FRONT;
    }
    return SDL_GPU_CULLMODE_NONE;
}

SDL_GPULoadOp toSdl(ELoadOp op) {
    switch (op) {
        case ELoadOp::kLoad: return SDL_GPU_LOADOP_LOAD;
        case ELoadOp::kClear: return SDL_GPU_LOADOP_CLEAR;
        case ELoadOp::kDontCare: return SDL_GPU_LOADOP_DONT_CARE;
    }
    return SDL_GPU_LOADOP_CLEAR;
}

SDL_GPUStoreOp toSdl(EStoreOp op) {
    return op == EStoreOp::kStore ? SDL_GPU_STOREOP_STORE : SDL_GPU_STOREOP_DONT_CARE;
}

SDL_GPUColorTargetBlendState makeBlendState(EBlendPreset preset) {
    SDL_GPUColorTargetBlendState state{};
    if (preset == EBlendPreset::kNone) {
        return state;
    }
    state.enable_blend = true;
    state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    if (preset == EBlendPreset::kAlpha) {
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    } else {
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    }
    return state;
}

// Veriyi CPU-görünür bir upload transfer buffer'ına kopyalar. Çağıran SDL_ReleaseGPUTransferBuffer eder.
SDL_GPUTransferBuffer* makeUploadTransfer(SDL_GPUDevice* device, std::span<const std::byte> data) {
    const SDL_GPUTransferBufferCreateInfo info{.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = static_cast<Uint32>(data.size())};
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &info);
    if (transfer == nullptr) {
        return nullptr;
    }
    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (mapped == nullptr) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return nullptr;
    }
    std::memcpy(mapped, data.data(), data.size());
    SDL_UnmapGPUTransferBuffer(device, transfer);
    return transfer;
}

bool recordBufferUpload(SDL_GPUDevice* device, SDL_GPUCommandBuffer* commands, SDL_GPUBuffer* destination,
                        uint32_t offsetBytes, std::span<const std::byte> data, bool cycle) {
    SDL_GPUTransferBuffer* transfer = makeUploadTransfer(device, data);
    if (transfer == nullptr) {
        return false;
    }
    SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(commands);
    const SDL_GPUTransferBufferLocation source{.transfer_buffer = transfer, .offset = 0};
    const SDL_GPUBufferRegion region{.buffer = destination, .offset = offsetBytes, .size = static_cast<Uint32>(data.size())};
    SDL_UploadToGPUBuffer(pass, &source, &region, cycle);
    SDL_EndGPUCopyPass(pass);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return true;
}

}  // namespace

// ===========================================================================
// CSdlGpuCommandList
// ===========================================================================

void CSdlGpuCommandList::begin(SDL_GPUCommandBuffer* commandBuffer) {
    m_commandBuffer = commandBuffer;
    m_pass = nullptr;
    m_pipelineBound = false;
}

void CSdlGpuCommandList::end() {
    endPass();
    m_commandBuffer = nullptr;
}

void CSdlGpuCommandList::markAllDirty() {
    for (auto& stage : m_uniforms) {
        for (UniformSlot_t& slot : stage) {
            slot.dirty = slot.size > 0;
        }
    }
    for (auto& stage : m_textures) {
        for (TextureSlot_t& slot : stage) {
            slot.dirty = slot.texture != nullptr && slot.sampler != nullptr;
        }
    }
}

void CSdlGpuCommandList::beginPass(const RenderPassDesc_t& pass) {
    if (m_commandBuffer == nullptr) {
        return;
    }
    endPass();

    std::array<SDL_GPUColorTargetInfo, kMaxColorTargets> colors{};
    const uint32_t colorCount = std::min(pass.colorCount, kMaxColorTargets);
    for (uint32_t i = 0; i < colorCount; ++i) {
        const ColorAttachment_t& attachment = pass.colors[i];
        const SdlTexture_t* texture = m_device.find(attachment.texture);
        if (texture == nullptr) {
            m_device.fail("beginPass: geçersiz color attachment");
            return;
        }
        colors[i].texture = texture->texture;
        colors[i].clear_color = SDL_FColor{attachment.clearColor.r, attachment.clearColor.g, attachment.clearColor.b, attachment.clearColor.a};
        colors[i].load_op = toSdl(attachment.load);
        colors[i].store_op = toSdl(attachment.store);
    }

    SDL_GPUDepthStencilTargetInfo depth{};
    bool hasDepth = false;
    if (pass.depth.texture.valid()) {
        const SdlTexture_t* texture = m_device.find(pass.depth.texture);
        if (texture == nullptr) {
            m_device.fail("beginPass: geçersiz depth attachment");
            return;
        }
        depth.texture = texture->texture;
        depth.clear_depth = pass.depth.clearDepth;
        depth.load_op = toSdl(pass.depth.load);
        depth.store_op = toSdl(pass.depth.store);
        depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        hasDepth = true;
    }

    m_pass = SDL_BeginGPURenderPass(m_commandBuffer, colorCount > 0 ? colors.data() : nullptr, colorCount, hasDepth ? &depth : nullptr);
    m_pipelineBound = false;
    markAllDirty();
}

void CSdlGpuCommandList::recordNative(const RenderPassDesc_t& pass, RenderOverlayFn fn, void* user) {
    if (m_commandBuffer == nullptr || fn == nullptr) {
        return;
    }
    endPass();
    fn(user, m_commandBuffer, nullptr);  // pass dışı: veri yükleme
    beginPass(pass);
    if (m_pass != nullptr) {
        fn(user, m_commandBuffer, m_pass);
    }
    endPass();
}

void CSdlGpuCommandList::endPass() {
    if (m_pass != nullptr) {
        SDL_EndGPURenderPass(m_pass);
        m_pass = nullptr;
        m_pipelineBound = false;
    }
}

void CSdlGpuCommandList::bindPipeline(PipelineHandle_t pipeline) {
    const SdlPipeline_t* record = m_device.find(pipeline);
    if (m_pass == nullptr || record == nullptr) {
        return;
    }
    SDL_BindGPUGraphicsPipeline(m_pass, record->pipeline);
    m_uniformCounts = {record->vertexUniformCount, record->fragmentUniformCount};
    m_pipelineBound = true;
    markAllDirty();  // pipeline değişince backend'ler bağlamaları sıfırlayabilir; kalıcı durumu yeniden gönder
}

void CSdlGpuCommandList::bindVertexBuffer(uint32_t slot, BufferHandle_t buffer, uint32_t offsetBytes) {
    const SdlBuffer_t* record = m_device.find(buffer);
    if (m_pass == nullptr || record == nullptr) {
        return;
    }
    const SDL_GPUBufferBinding binding{.buffer = record->buffer, .offset = offsetBytes};
    SDL_BindGPUVertexBuffers(m_pass, slot, &binding, 1);
}

void CSdlGpuCommandList::bindIndexBuffer(BufferHandle_t buffer, EIndexType type, uint32_t offsetBytes) {
    const SdlBuffer_t* record = m_device.find(buffer);
    if (m_pass == nullptr || record == nullptr) {
        return;
    }
    const SDL_GPUBufferBinding binding{.buffer = record->buffer, .offset = offsetBytes};
    SDL_BindGPUIndexBuffer(m_pass, &binding, type == EIndexType::kUint16 ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);
    m_indexType = type;
}

void CSdlGpuCommandList::setUniformBytes(EShaderStage stage, uint32_t slot, std::span<const std::byte> data) {
    if (slot >= kMaxUniformSlots || data.size() > kMaxUniformBytes) {
        m_device.fail("setUniformBytes: slot veya boyut sınırı aşıldı");
        return;
    }
    UniformSlot_t& target = m_uniforms[static_cast<size_t>(stage)][slot];
    std::memcpy(target.data.data(), data.data(), data.size());
    target.size = static_cast<uint32_t>(data.size());
    target.dirty = true;
}

void CSdlGpuCommandList::bindTexture(EShaderStage stage, uint32_t slot, TextureHandle_t texture, SamplerHandle_t sampler) {
    const SdlTexture_t* textureRecord = m_device.find(texture);
    const SdlSampler_t* samplerRecord = m_device.find(sampler);
    if (slot >= kMaxTextureSlots || textureRecord == nullptr || samplerRecord == nullptr) {
        m_device.fail("bindTexture: geçersiz texture/sampler/slot");
        return;
    }
    TextureSlot_t& target = m_textures[static_cast<size_t>(stage)][slot];
    target = {.texture = textureRecord->texture, .sampler = samplerRecord->sampler, .dirty = true};
}

void CSdlGpuCommandList::setViewport(const Viewport_t& viewport) {
    if (m_pass == nullptr) {
        return;
    }
    const SDL_GPUViewport sdlViewport{viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth};
    SDL_SetGPUViewport(m_pass, &sdlViewport);
}

void CSdlGpuCommandList::setScissor(const ScissorRect_t& scissor) {
    if (m_pass == nullptr) {
        return;
    }
    const SDL_Rect rect{scissor.x, scissor.y, static_cast<int>(scissor.width), static_cast<int>(scissor.height)};
    SDL_SetGPUScissor(m_pass, &rect);
}

bool CSdlGpuCommandList::prepareDraw() {
    if (m_pass == nullptr || !m_pipelineBound) {
        return false;
    }
    for (size_t stage = 0; stage < kStageCount; ++stage) {
        const uint32_t uniformCount = std::min(m_uniformCounts[stage], kMaxUniformSlots);
        for (uint32_t slot = 0; slot < uniformCount; ++slot) {
            UniformSlot_t& uniform = m_uniforms[stage][slot];
            if (!uniform.dirty || uniform.size == 0) {
                continue;
            }
            if (stage == static_cast<size_t>(EShaderStage::kVertex)) {
                SDL_PushGPUVertexUniformData(m_commandBuffer, slot, uniform.data.data(), uniform.size);
            } else {
                SDL_PushGPUFragmentUniformData(m_commandBuffer, slot, uniform.data.data(), uniform.size);
            }
            uniform.dirty = false;
        }
        for (uint32_t slot = 0; slot < kMaxTextureSlots; ++slot) {
            TextureSlot_t& texture = m_textures[stage][slot];
            if (!texture.dirty) {
                continue;
            }
            const SDL_GPUTextureSamplerBinding binding{.texture = texture.texture, .sampler = texture.sampler};
            if (stage == static_cast<size_t>(EShaderStage::kVertex)) {
                SDL_BindGPUVertexSamplers(m_pass, slot, &binding, 1);
            } else {
                SDL_BindGPUFragmentSamplers(m_pass, slot, &binding, 1);
            }
            texture.dirty = false;
        }
    }
    return true;
}

void CSdlGpuCommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex) {
    if (prepareDraw()) {
        SDL_DrawGPUPrimitives(m_pass, vertexCount, instanceCount, firstVertex, 0);
    }
}

void CSdlGpuCommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex) {
    if (prepareDraw()) {
        SDL_DrawGPUIndexedPrimitives(m_pass, indexCount, instanceCount, firstIndex, baseVertex, 0);
    }
}

void CSdlGpuCommandList::updateBuffer(BufferHandle_t buffer, uint32_t offsetBytes, std::span<const std::byte> data, EUpdateMode mode) {
    const SdlBuffer_t* record = m_device.find(buffer);
    if (m_commandBuffer == nullptr || m_pass != nullptr || record == nullptr || data.empty() ||
        offsetBytes + data.size() > record->size) {
        m_device.fail("updateBuffer: pass içinde, geçersiz buffer veya aralık dışı");
        return;
    }
    if (!recordBufferUpload(m_device.sdlDevice(), m_commandBuffer, record->buffer, offsetBytes, data, mode == EUpdateMode::kDiscard)) {
        m_device.fail(std::string{"updateBuffer: transfer başarısız: "} + SDL_GetError());
    }
}

// ===========================================================================
// CSdlGpuDevice
// ===========================================================================

CSdlGpuDevice::CSdlGpuDevice(SdlGpuConfig_t config) : m_config(config) {
    m_caps = {.backend = config.backend,
              .backendName = config.backendName,
              .shaderLanguage = config.language,
              .maxTextureSize = 8192,
              .depthZeroToOne = true};
}

CSdlGpuDevice::~CSdlGpuDevice() {
    shutdown();
}

void CSdlGpuDevice::fail(const std::string& message) {
    m_lastError = message;
    std::cerr << "render/rhi: " << message << "\n";
}

bool CSdlGpuDevice::initialize(const RhiDeviceDesc_t& desc) {
    if (desc.surface.kind != ENativeWindowKind::kSdlWindow || desc.surface.handle == nullptr) {
        fail("SDL_GPU backend yalnızca SDL_Window* yüzeyi kabul eder");
        return false;
    }
    m_window = static_cast<SDL_Window*>(desc.surface.handle);
    m_device = SDL_CreateGPUDevice(m_config.shaderFormats, desc.debugLayer, m_config.driverName);
    if (m_device == nullptr) {
        fail(std::string{"SDL_CreateGPUDevice başarısız: "} + SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(m_device, m_window)) {
        fail(std::string{"pencere GPU'ya bağlanamadı: "} + SDL_GetError());
        shutdown();
        return false;
    }
    if (!applyPresentMode(desc.vsync)) {
        shutdown();
        return false;
    }
    m_swapchainFormat = fromSdl(SDL_GetGPUSwapchainTextureFormat(m_device, m_window));
    return true;
}

bool CSdlGpuDevice::applyPresentMode(bool vsync) {
    SDL_GPUPresentMode mode = SDL_GPU_PRESENTMODE_VSYNC;
    if (!vsync) {
        if (SDL_WindowSupportsGPUPresentMode(m_device, m_window, SDL_GPU_PRESENTMODE_IMMEDIATE)) {
            mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        } else if (SDL_WindowSupportsGPUPresentMode(m_device, m_window, SDL_GPU_PRESENTMODE_MAILBOX)) {
            mode = SDL_GPU_PRESENTMODE_MAILBOX;
        }
    }
    if (!SDL_SetGPUSwapchainParameters(m_device, m_window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode)) {
        fail(std::string{"swapchain parametreleri ayarlanamadı: "} + SDL_GetError());
        return false;
    }
    return true;
}

void CSdlGpuDevice::setVsync(bool enabled) {
    if (m_device != nullptr) {
        (void)applyPresentMode(enabled);
    }
}

void CSdlGpuDevice::shutdown() {
    if (m_device == nullptr) {
        return;
    }
    SDL_WaitForGPUIdle(m_device);
    m_pipelines.drain([this](SdlPipeline_t& item) { SDL_ReleaseGPUGraphicsPipeline(m_device, item.pipeline); });
    m_shaders.drain([this](SdlShader_t& item) { SDL_ReleaseGPUShader(m_device, item.shader); });
    m_samplers.drain([this](SdlSampler_t& item) { SDL_ReleaseGPUSampler(m_device, item.sampler); });
    m_textures.drain([this](SdlTexture_t& item) {
        if (!item.external) {
            SDL_ReleaseGPUTexture(m_device, item.texture);
        }
    });
    m_buffers.drain([this](SdlBuffer_t& item) { SDL_ReleaseGPUBuffer(m_device, item.buffer); });
    if (m_window != nullptr) {
        SDL_ReleaseWindowFromGPUDevice(m_device, m_window);
    }
    SDL_DestroyGPUDevice(m_device);
    m_device = nullptr;
    m_window = nullptr;
    m_frameActive = false;
    m_swapchainHandle = {};
}

void CSdlGpuDevice::waitIdle() {
    if (m_device != nullptr) {
        SDL_WaitForGPUIdle(m_device);
    }
}

// --- kaynak oluşturma -------------------------------------------------------

BufferHandle_t CSdlGpuDevice::createBuffer(const BufferDesc_t& desc, std::span<const std::byte> initialData) {
    if (m_device == nullptr || desc.sizeBytes == 0 || initialData.size() > desc.sizeBytes) {
        fail(std::string{"createBuffer: geçersiz parametre ("} + desc.debugName + ")");
        return {};
    }
    SDL_GPUBufferCreateInfo info{};
    info.usage = (hasFlag(desc.usage, EBufferUsage::kVertex) ? SDL_GPU_BUFFERUSAGE_VERTEX : 0) |
                 (hasFlag(desc.usage, EBufferUsage::kIndex) ? SDL_GPU_BUFFERUSAGE_INDEX : 0);
    info.size = desc.sizeBytes;
    SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(m_device, &info);
    if (buffer == nullptr) {
        fail(std::string{"createBuffer başarısız: "} + SDL_GetError());
        return {};
    }
    if (!initialData.empty()) {
        SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(m_device);
        const bool ok = commands != nullptr && recordBufferUpload(m_device, commands, buffer, 0, initialData, false);
        if (commands != nullptr) {
            SDL_SubmitGPUCommandBuffer(commands);
        }
        if (!ok) {
            SDL_ReleaseGPUBuffer(m_device, buffer);
            fail(std::string{"createBuffer: yükleme başarısız: "} + SDL_GetError());
            return {};
        }
    }
    return m_buffers.create({.buffer = buffer, .size = desc.sizeBytes});
}

TextureHandle_t CSdlGpuDevice::createTexture(const TextureDesc_t& desc, std::span<const std::byte> initialPixels) {
    const uint32_t pixelBytes = bytesPerPixel(desc.format);
    const size_t expectedBytes = static_cast<size_t>(desc.width) * desc.height * pixelBytes;
    if (m_device == nullptr || desc.width == 0 || desc.height == 0 || pixelBytes == 0 ||
        (!initialPixels.empty() && initialPixels.size() < expectedBytes)) {
        fail(std::string{"createTexture: geçersiz parametre ("} + desc.debugName + ")");
        return {};
    }

    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = toSdl(desc.format);
    info.usage = 0;
    if (hasFlag(desc.usage, ETextureUsage::kSampled)) info.usage |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
    if (hasFlag(desc.usage, ETextureUsage::kColorTarget) || desc.generateMips) info.usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    if (hasFlag(desc.usage, ETextureUsage::kDepthTarget)) info.usage |= SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    info.width = desc.width;
    info.height = desc.height;
    info.layer_count_or_depth = 1;
    info.num_levels = desc.generateMips ? static_cast<Uint32>(std::bit_width(std::max(desc.width, desc.height))) : 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    SDL_GPUTexture* texture = SDL_CreateGPUTexture(m_device, &info);
    if (texture == nullptr) {
        fail(std::string{"createTexture başarısız: "} + SDL_GetError());
        return {};
    }

    if (!initialPixels.empty()) {
        SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(m_device);
        SDL_GPUTransferBuffer* transfer = makeUploadTransfer(m_device, initialPixels.first(expectedBytes));
        if (commands == nullptr || transfer == nullptr) {
            if (transfer != nullptr) SDL_ReleaseGPUTransferBuffer(m_device, transfer);
            if (commands != nullptr) SDL_SubmitGPUCommandBuffer(commands);
            SDL_ReleaseGPUTexture(m_device, texture);
            fail(std::string{"createTexture: yükleme başarısız: "} + SDL_GetError());
            return {};
        }
        SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(commands);
        const SDL_GPUTextureTransferInfo source{.transfer_buffer = transfer, .offset = 0, .pixels_per_row = desc.width, .rows_per_layer = desc.height};
        const SDL_GPUTextureRegion region{.texture = texture, .mip_level = 0, .layer = 0, .x = 0, .y = 0, .z = 0, .w = desc.width, .h = desc.height, .d = 1};
        SDL_UploadToGPUTexture(pass, &source, &region, false);
        SDL_EndGPUCopyPass(pass);
        if (desc.generateMips) {
            SDL_GenerateMipmapsForGPUTexture(commands, texture);
        }
        SDL_SubmitGPUCommandBuffer(commands);
        SDL_ReleaseGPUTransferBuffer(m_device, transfer);
    }
    return m_textures.create({.texture = texture, .width = desc.width, .height = desc.height, .format = desc.format});
}

SamplerHandle_t CSdlGpuDevice::createSampler(const SamplerDesc_t& desc) {
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = toSdl(desc.minFilter);
    info.mag_filter = toSdl(desc.magFilter);
    info.mipmap_mode = toSdlMip(desc.mipFilter);
    info.address_mode_u = toSdl(desc.addressU);
    info.address_mode_v = toSdl(desc.addressV);
    info.address_mode_w = toSdl(desc.addressW);
    info.max_anisotropy = desc.maxAnisotropy;
    info.enable_anisotropy = desc.maxAnisotropy > 1.0f;
    info.max_lod = 1000.0f;
    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(m_device, &info);
    if (sampler == nullptr) {
        fail(std::string{"createSampler başarısız: "} + SDL_GetError());
        return {};
    }
    return m_samplers.create({.sampler = sampler});
}

ShaderHandle_t CSdlGpuDevice::createShader(const ShaderDesc_t& desc) {
    if (desc.language != m_config.language) {
        fail(std::string{"createShader: backend bu shader dilini desteklemiyor ("} + desc.debugName + ")");
        return {};
    }
    SDL_GPUShaderCreateInfo info{};
    info.code_size = desc.source.size();
    info.code = reinterpret_cast<const Uint8*>(desc.source.data());
    info.entrypoint = desc.entryPoint;
    info.format = m_config.shaderFormats;  // tek dil: yapılandırmadaki format
    info.stage = desc.stage == EShaderStage::kVertex ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_uniform_buffers = desc.uniformBufferCount;
    info.num_samplers = desc.samplerCount;
    SDL_GPUShader* shader = SDL_CreateGPUShader(m_device, &info);
    if (shader == nullptr) {
        fail(std::string{"shader derlenemedi ("} + desc.entryPoint + "): " + SDL_GetError());
        return {};
    }
    return m_shaders.create({.shader = shader, .uniformBufferCount = desc.uniformBufferCount, .stage = desc.stage});
}

PipelineHandle_t CSdlGpuDevice::createGraphicsPipeline(const GraphicsPipelineDesc_t& desc) {
    const SdlShader_t* vertex = m_shaders.get(desc.vertexShader);
    const SdlShader_t* fragment = m_shaders.get(desc.fragmentShader);
    if (vertex == nullptr || fragment == nullptr) {
        fail(std::string{"createGraphicsPipeline: geçersiz shader ("} + desc.debugName + ")");
        return {};
    }

    std::array<SDL_GPUVertexAttribute, kMaxVertexAttributes> attributes{};
    const uint32_t attributeCount = std::min(desc.vertexLayout.attributeCount, kMaxVertexAttributes);
    for (uint32_t i = 0; i < attributeCount; ++i) {
        const VertexAttribute_t& attribute = desc.vertexLayout.attributes[i];
        attributes[i] = {.location = attribute.location, .buffer_slot = 0, .format = toSdl(attribute.format), .offset = attribute.offset};
    }
    SDL_GPUVertexBufferDescription vertexBuffer{};
    vertexBuffer.slot = 0;
    vertexBuffer.pitch = desc.vertexLayout.stride;
    vertexBuffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    std::array<SDL_GPUColorTargetDescription, kMaxColorTargets> targets{};
    const uint32_t targetCount = std::min(desc.colorTargetCount, kMaxColorTargets);
    for (uint32_t i = 0; i < targetCount; ++i) {
        targets[i].format = toSdl(desc.colorFormats[i]);
        targets[i].blend_state = makeBlendState(desc.blend);
    }

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vertex->shader;
    info.fragment_shader = fragment->shader;
    if (desc.vertexLayout.stride > 0) {
        info.vertex_input_state.vertex_buffer_descriptions = &vertexBuffer;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attributes.data();
        info.vertex_input_state.num_vertex_attributes = attributeCount;
    }
    info.primitive_type = desc.topology == EPrimitiveTopology::kLineList ? SDL_GPU_PRIMITIVETYPE_LINELIST : SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = toSdl(desc.cull);
    info.rasterizer_state.front_face = desc.frontCounterClockwise ? SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE : SDL_GPU_FRONTFACE_CLOCKWISE;
    info.rasterizer_state.depth_bias_constant_factor = desc.depthBiasConstant;
    info.rasterizer_state.depth_bias_slope_factor = desc.depthBiasSlope;
    info.rasterizer_state.enable_depth_bias = desc.depthBiasConstant != 0.0f || desc.depthBiasSlope != 0.0f;
    info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    info.depth_stencil_state.compare_op = toSdl(desc.depth.compare);
    info.depth_stencil_state.enable_depth_test = desc.depth.test;
    info.depth_stencil_state.enable_depth_write = desc.depth.write;
    info.target_info.color_target_descriptions = targetCount > 0 ? targets.data() : nullptr;
    info.target_info.num_color_targets = targetCount;
    info.target_info.depth_stencil_format = toSdl(desc.depthFormat);
    info.target_info.has_depth_stencil_target = desc.depthFormat != EPixelFormat::kUndefined;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(m_device, &info);
    if (pipeline == nullptr) {
        fail(std::string{"pipeline oluşturulamadı ("} + desc.debugName + "): " + SDL_GetError());
        return {};
    }
    return m_pipelines.create({.pipeline = pipeline,
                               .vertexUniformCount = vertex->uniformBufferCount,
                               .fragmentUniformCount = fragment->uniformBufferCount});
}

// --- yıkım ------------------------------------------------------------------

void CSdlGpuDevice::destroy(BufferHandle_t handle) {
    if (auto item = m_buffers.destroy(handle)) SDL_ReleaseGPUBuffer(m_device, item->buffer);
}

void CSdlGpuDevice::destroy(TextureHandle_t handle) {
    if (auto item = m_textures.destroy(handle); item && !item->external) SDL_ReleaseGPUTexture(m_device, item->texture);
}

void CSdlGpuDevice::destroy(SamplerHandle_t handle) {
    if (auto item = m_samplers.destroy(handle)) SDL_ReleaseGPUSampler(m_device, item->sampler);
}

void CSdlGpuDevice::destroy(ShaderHandle_t handle) {
    if (auto item = m_shaders.destroy(handle)) SDL_ReleaseGPUShader(m_device, item->shader);
}

void CSdlGpuDevice::destroy(PipelineHandle_t handle) {
    if (auto item = m_pipelines.destroy(handle)) SDL_ReleaseGPUGraphicsPipeline(m_device, item->pipeline);
}

// --- frame ------------------------------------------------------------------

std::optional<RhiFrameInfo_t> CSdlGpuDevice::beginFrame() {
    if (m_device == nullptr || m_frameActive) {
        return std::nullopt;
    }
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(m_device);
    if (commands == nullptr) {
        return std::nullopt;
    }
    Uint32 width = 0;
    Uint32 height = 0;
    SDL_GPUTexture* swapchain = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, m_window, &swapchain, &width, &height) || swapchain == nullptr) {
        // Pencere küçülmüş olabilir; command buffer yine de gönderilmeli.
        SDL_SubmitGPUCommandBuffer(commands);
        return std::nullopt;
    }
    m_frameCommands = commands;
    m_commands.begin(commands);
    m_swapchainHandle = m_textures.create({.texture = swapchain, .width = width, .height = height, .format = m_swapchainFormat, .external = true});
    m_frameActive = true;
    return RhiFrameInfo_t{.backbuffer = m_swapchainHandle, .width = width, .height = height, .format = m_swapchainFormat};
}

void CSdlGpuDevice::endFrame() {
    if (!m_frameActive) {
        return;
    }
    m_commands.end();
    SDL_SubmitGPUCommandBuffer(m_frameCommands);
    m_frameCommands = nullptr;
    destroy(std::exchange(m_swapchainHandle, TextureHandle_t{}));
    m_frameActive = false;
}

}  // namespace game::rhi
