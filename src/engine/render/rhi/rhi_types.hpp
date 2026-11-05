#pragma once

// RHI (render hardware interface): the low-level types shared by the backends (SDL_GPU/Metal, later D3D11).
// The upper layers (passes, render_system) only talk through these types; they never see any backend header.

#include "engine/interface/render_types.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include <glm/vec4.hpp>

namespace game::rhi {

struct BufferTag_t;
struct TextureTag_t;
struct SamplerTag_t;
struct ShaderTag_t;
struct PipelineTag_t;
using BufferHandle_t = RenderHandle_t<BufferTag_t>;
using TextureHandle_t = RenderHandle_t<TextureTag_t>;
using SamplerHandle_t = RenderHandle_t<SamplerTag_t>;
using ShaderHandle_t = RenderHandle_t<ShaderTag_t>;
using PipelineHandle_t = RenderHandle_t<PipelineTag_t>;

inline constexpr uint32_t kMaxColorTargets = 4;
inline constexpr uint32_t kMaxVertexAttributes = 8;
inline constexpr uint32_t kMaxUniformSlots = 4;
inline constexpr uint32_t kMaxTextureSlots = 8;
inline constexpr uint32_t kMaxUniformBytes = 8192;  // per slot (a bone block with 128 joints is 8 KB)

// --- bit flag enums ---
template <typename E>
inline constexpr bool kIsBitFlags = false;

template <typename E>
    requires kIsBitFlags<E>
[[nodiscard]] constexpr E operator|(E a, E b) {
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) | static_cast<std::underlying_type_t<E>>(b));
}

template <typename E>
    requires kIsBitFlags<E>
[[nodiscard]] constexpr bool hasFlag(E value, E flag) {
    return (static_cast<std::underlying_type_t<E>>(value) & static_cast<std::underlying_type_t<E>>(flag)) != 0;
}

// --- formats ---
enum class EPixelFormat : uint8_t {
    kUndefined,
    kR8Unorm,
    kRgba8Unorm,
    kRgba8UnormSrgb,
    kBgra8Unorm,
    kBgra8UnormSrgb,
    kRgba16Float,
    kD32Float,
};

[[nodiscard]] constexpr bool isDepthFormat(EPixelFormat format) {
    return format == EPixelFormat::kD32Float;
}

[[nodiscard]] constexpr bool isSrgbFormat(EPixelFormat format) {
    return format == EPixelFormat::kRgba8UnormSrgb || format == EPixelFormat::kBgra8UnormSrgb;
}

[[nodiscard]] constexpr uint32_t bytesPerPixel(EPixelFormat format) {
    switch (format) {
        case EPixelFormat::kUndefined: return 0;
        case EPixelFormat::kR8Unorm: return 1;
        case EPixelFormat::kRgba16Float: return 8;
        default: return 4;
    }
}

enum class EBufferUsage : uint8_t {
    kVertex = 1,
    kIndex = 2,
};
template <>
inline constexpr bool kIsBitFlags<EBufferUsage> = true;

enum class ETextureUsage : uint8_t {
    kSampled = 1,
    kColorTarget = 2,
    kDepthTarget = 4,
};
template <>
inline constexpr bool kIsBitFlags<ETextureUsage> = true;

enum class EShaderStage : uint8_t { kVertex, kFragment };
enum class EShaderLanguage : uint8_t { kMsl, kHlsl, kSpirv };
enum class EVertexFormat : uint8_t { kFloat2, kFloat3, kFloat4, kUByte4Norm };  // kUByte4Norm: 4 bytes -> float4 (0..1)
enum class EFilter : uint8_t { kNearest, kLinear };
enum class EAddressMode : uint8_t { kRepeat, kClamp, kMirror };
enum class EPrimitiveTopology : uint8_t { kTriangleList, kLineList };
enum class ECullMode : uint8_t { kNone, kBack, kFront };
enum class EDepthCompare : uint8_t { kNever, kLess, kLessEqual, kAlways };
enum class EBlendPreset : uint8_t { kNone, kAlpha, kAdditive };
enum class ELoadOp : uint8_t { kLoad, kClear, kDontCare };
enum class EStoreOp : uint8_t { kStore, kDontCare };

// kPreserve: old contents are kept. kDiscard: old contents may be thrown away (dynamic data rewritten from scratch every frame;
// if the GPU is still reading the old frame, the backend gives a new copy).
enum class EUpdateMode : uint8_t { kPreserve, kDiscard };

[[nodiscard]] constexpr uint32_t vertexFormatSize(EVertexFormat format) {
    switch (format) {
        case EVertexFormat::kFloat2: return 8;
        case EVertexFormat::kFloat3: return 12;
        case EVertexFormat::kFloat4: return 16;
        case EVertexFormat::kUByte4Norm: return 4;
    }
    return 0;
}

// --- resource descriptions ---
struct BufferDesc_t {
    uint32_t sizeBytes = 0;
    EBufferUsage usage = EBufferUsage::kVertex;
    const char* debugName = "";
};

struct TextureDesc_t {
    uint32_t width = 0;
    uint32_t height = 0;
    EPixelFormat format = EPixelFormat::kRgba8Unorm;
    ETextureUsage usage = ETextureUsage::kSampled;
    bool generateMips = false;  // a mip chain is generated after the initial data is uploaded
    const char* debugName = "";
};

struct SamplerDesc_t {
    EFilter minFilter = EFilter::kLinear;
    EFilter magFilter = EFilter::kLinear;
    EFilter mipFilter = EFilter::kLinear;
    EAddressMode addressU = EAddressMode::kClamp;
    EAddressMode addressV = EAddressMode::kClamp;
    EAddressMode addressW = EAddressMode::kClamp;
    float maxAnisotropy = 1.0f;  // >1 means anisotropic filtering
};

struct ShaderDesc_t {
    EShaderStage stage = EShaderStage::kVertex;
    EShaderLanguage language = EShaderLanguage::kMsl;
    std::string_view source;
    const char* entryPoint = "main";
    uint32_t uniformBufferCount = 0;
    uint32_t samplerCount = 0;
    const char* debugName = "";
};

struct VertexAttribute_t {
    uint32_t location = 0;
    EVertexFormat format = EVertexFormat::kFloat3;
    uint32_t offset = 0;
};

// A single vertex stream (slot 0). stride == 0 means there is no vertex input (like a fullscreen triangle).
struct VertexLayout_t {
    uint32_t stride = 0;
    uint32_t attributeCount = 0;
    std::array<VertexAttribute_t, kMaxVertexAttributes> attributes{};
};

struct DepthState_t {
    bool test = false;
    bool write = false;
    EDepthCompare compare = EDepthCompare::kLess;
};

struct GraphicsPipelineDesc_t {
    ShaderHandle_t vertexShader;
    ShaderHandle_t fragmentShader;
    VertexLayout_t vertexLayout;
    EPrimitiveTopology topology = EPrimitiveTopology::kTriangleList;
    ECullMode cull = ECullMode::kNone;
    bool frontCounterClockwise = true;
    DepthState_t depth;
    EBlendPreset blend = EBlendPreset::kNone;
    uint32_t colorTargetCount = 0;
    std::array<EPixelFormat, kMaxColorTargets> colorFormats{};
    EPixelFormat depthFormat = EPixelFormat::kUndefined;
    float depthBiasConstant = 0.0f;
    float depthBiasSlope = 0.0f;
    const char* debugName = "";
};

// --- render pass ---
struct ColorAttachment_t {
    TextureHandle_t texture;
    ELoadOp load = ELoadOp::kClear;
    EStoreOp store = EStoreOp::kStore;
    glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
};

struct DepthAttachment_t {
    TextureHandle_t texture;  // no depth target if invalid
    ELoadOp load = ELoadOp::kClear;
    EStoreOp store = EStoreOp::kDontCare;
    float clearDepth = 1.0f;
};

struct RenderPassDesc_t {
    std::array<ColorAttachment_t, kMaxColorTargets> colors{};
    uint32_t colorCount = 0;
    DepthAttachment_t depth;
    const char* debugName = "";
};

struct Viewport_t {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct ScissorRect_t {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

}  // namespace game::rhi
