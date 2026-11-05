#pragma once

// Renderer001 common types. Completely independent of the backend (Metal / D3D11 / ...).
// This header lives only in the interface/ layer: it has no dependency on engine or render internals.
//
// Conventions (same in all backends):
//   - Right-handed coordinate system, +Y up, yaw=0 looks toward -Z.
//   - Projection depth 0..1 (glm::perspectiveRH_ZO). Matrices are glm column-major.
//   - Color space: MaterialDesc_t::baseColor / emissive are entered as "artistic" sRGB; the renderer converts them to linear.
//     Ambient/light colors (EnvironmentDesc_t, LightDesc_t) are in linear space.
//   - Light directions are the "direction the light travels" (from the light source toward the scene).
//   - Units: meters, seconds, radians.

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace game {

// ---------------------------------------------------------------------------
// Handles: index + generation. generation == 0 is invalid (default-constructed = invalid).
// Reusing a deleted handle is safe: the old handle does not resolve and the call is ignored.
// ---------------------------------------------------------------------------
template <typename Tag>
struct RenderHandle_t {
    uint32_t index = 0;
    uint32_t generation = 0;

    [[nodiscard]] constexpr bool valid() const { return generation != 0; }
    [[nodiscard]] constexpr explicit operator bool() const { return valid(); }
    friend constexpr bool operator==(const RenderHandle_t&, const RenderHandle_t&) = default;
};

struct MeshTag_t;
struct TextureTag_t;
struct MaterialTag_t;
using MeshHandle_t = RenderHandle_t<MeshTag_t>;
using TextureHandle_t = RenderHandle_t<TextureTag_t>;
using MaterialHandle_t = RenderHandle_t<MaterialTag_t>;

inline constexpr uint32_t kMaxLights = 16;           // dynamic lights per frame (sun excluded)
inline constexpr uint32_t kMaxDebugLines = 65536;    // upper limit of debug lines per frame
inline constexpr uint32_t kMaxBones = 128;           // joints per draw for skinned meshes (uniform block is 8 KB)

// ---------------------------------------------------------------------------
// Backend / surface
// ---------------------------------------------------------------------------
enum class ERenderBackend : uint8_t {
    kAuto,    // the platform's default (macOS: Metal)
    kMetal,
    kD3D11,   // not implemented yet; initialize() returns an error
    kD3D12,
    kVulkan,
};

// Native window handle kind. Describes the handle the platform module provides; the renderer does not depend on SDL.
enum class ENativeWindowKind : uint8_t {
    kSdlWindow,      // SDL_Window*
    kWin32Hwnd,      // HWND
    kCocoaNsWindow,  // NSWindow*
};

struct RenderSurface_t {
    ENativeWindowKind kind = ENativeWindowKind::kSdlWindow;
    void* handle = nullptr;
};

enum class EShadowQuality : uint8_t {
    kOff,
    kLow,     // 1024
    kMedium,  // 2048
    kHigh,    // 4096
};

struct RenderSettings_t {
    bool vsync = true;
    EShadowQuality shadowQuality = EShadowQuality::kHigh;
    float shadowExtent = 45.0f;  // half-width of the area the sun shadow covers around the camera (meters)
    float renderScale = 1.0f;    // internal resolution multiplier (0.25..2). The post pass scales to the screen.
};

// Recording into the backend's command buffer from outside (like the Dear ImGui backend). It is the last step drawn into the swapchain in the frame.
// Called twice: first outside a render pass (nativeRenderPass == nullptr; uploading data to the GPU), then inside the pass.
// SDL_GPU backend: nativeCommandBuffer = SDL_GPUCommandBuffer*, nativeRenderPass = SDL_GPURenderPass*.
using RenderOverlayFn = void (*)(void* user, void* nativeCommandBuffer, void* nativeRenderPass);

struct RendererDesc_t {
    RenderSurface_t surface;
    ERenderBackend backend = ERenderBackend::kAuto;
    RenderSettings_t settings;
    bool debugLayer = false;  // backend validation layer (slow)
};

struct RenderCaps_t {
    ERenderBackend backend = ERenderBackend::kAuto;
    const char* backendName = "";
    const char* shaderLanguage = "";
    uint32_t maxTextureSize = 0;
    uint32_t maxLights = kMaxLights;
    bool depthZeroToOne = true;
};

struct RenderStats_t {
    uint32_t drawsSubmitted = 0;
    uint32_t drawsCulled = 0;
    uint32_t drawCalls = 0;
    uint32_t shadowDrawCalls = 0;
    uint32_t triangles = 0;
    uint32_t lights = 0;
    uint32_t debugLines = 0;
    uint32_t passes = 0;
    uint32_t meshes = 0;
    uint32_t textures = 0;
    uint32_t materials = 0;
};

// ---------------------------------------------------------------------------
// Mesh
// ---------------------------------------------------------------------------
enum class EVertexLayout : uint8_t {
    kPosNormalUv,      // float3 position, float3 normal, float2 uv = 32 bytes. Position is at offset 0 in every layout.
    kPosNormalUvSkin,  // kPosNormalUv + ubyte4 joints (raw indices) + float4 weights = 52 bytes (anim::SkinnedVertex_t). Requires DrawItem_t::bones.
};

enum class EIndexType : uint8_t {
    kUint16,
    kUint32,
};

[[nodiscard]] constexpr uint32_t vertexStride(EVertexLayout layout) {
    switch (layout) {
        case EVertexLayout::kPosNormalUv: return 32;
        case EVertexLayout::kPosNormalUvSkin: return 52;
    }
    return 0;
}

[[nodiscard]] constexpr uint32_t indexSize(EIndexType type) {
    return type == EIndexType::kUint16 ? 2u : 4u;
}

// After the call returns the renderer copies the data to the GPU; the spans are not kept after the call.
struct MeshDesc_t {
    const char* debugName = "";
    EVertexLayout layout = EVertexLayout::kPosNormalUv;
    std::span<const std::byte> vertexData;
    EIndexType indexType = EIndexType::kUint32;
    std::span<const std::byte> indexData;
};

// Builds a MeshDesc_t from typed vertex/index arrays. E.g.: makeMeshDesc<Vertex_t, uint32_t>("box", v, i)
template <typename Vertex, typename Index>
[[nodiscard]] MeshDesc_t makeMeshDesc(const char* name, std::span<const Vertex> vertices, std::span<const Index> indices,
                                      EVertexLayout layout = EVertexLayout::kPosNormalUv) {
    static_assert(std::is_trivially_copyable_v<Vertex>);
    static_assert(sizeof(Index) == 2 || sizeof(Index) == 4, "index must be 16 or 32 bit");
    return MeshDesc_t{
        .debugName = name,
        .layout = layout,
        .vertexData = std::as_bytes(vertices),
        .indexType = sizeof(Index) == 2 ? EIndexType::kUint16 : EIndexType::kUint32,
        .indexData = std::as_bytes(indices),
    };
}

// ---------------------------------------------------------------------------
// Texture
// ---------------------------------------------------------------------------
enum class ETextureFormat : uint8_t {
    kR8Unorm,
    kRgba8Unorm,      // linear data (normal map, mask...)
    kRgba8UnormSrgb,  // color (albedo)
    kRgba16Float,
};

struct TextureDesc_t {
    const char* debugName = "";
    uint32_t width = 0;
    uint32_t height = 0;
    ETextureFormat format = ETextureFormat::kRgba8UnormSrgb;
    bool generateMips = true;
    std::span<const std::byte> pixels;  // mip 0, rows tightly packed (width * pixel size)
};

// ---------------------------------------------------------------------------
// Material
// ---------------------------------------------------------------------------
enum class EShadingModel : uint8_t {
    kLit,      // Blinn-Phong + sun shadow + dynamic lights + fog
    kRimLit,   // kLit + body gradient and fresnel rim light (for characters)
    kUnlit,    // only color + emissive (fog applied)
};

enum class EBlendMode : uint8_t {
    kOpaque,
    kAlphaBlend,  // does not write depth, sorted back to front, does not cast shadows
};

struct MaterialDesc_t {
    const char* debugName = "";
    EShadingModel shading = EShadingModel::kLit;
    EBlendMode blend = EBlendMode::kOpaque;
    glm::vec4 baseColor{1.0f};       // sRGB, a = opacity (kAlphaBlend)
    TextureHandle_t baseColorTexture;  // if invalid: white
    glm::vec3 emissive{0.0f};        // linear, may be HDR
    float specularPower = 48.0f;
    float specularAmount = 0.18f;
    float rimAmount = 0.8f;          // only kRimLit
    float gridStrength = 0.0f;       // 0..1: checkered pattern on the world XZ plane (for the ground)
    bool doubleSided = true;
    bool castShadow = true;
};

// ---------------------------------------------------------------------------
// Frame / scene
// ---------------------------------------------------------------------------
struct FrameDesc_t {
    float timeSeconds = 0.0f;
    float deltaSeconds = 0.0f;
};

struct ViewDesc_t {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};  // perspectiveRH_ZO
};

// Linear colors. sunDirection: the direction the sunlight travels (does not need to be normalized).
struct EnvironmentDesc_t {
    glm::vec3 sunDirection{-0.5f, -0.7f, -0.35f};
    glm::vec3 sunColor{1.0f, 0.93f, 0.8f};
    float sunIntensity = 3.2f;
    glm::vec3 skyColor{0.12f, 0.30f, 0.75f};      // zenith + ambient upper hemisphere
    glm::vec3 horizonColor{0.60f, 0.72f, 0.88f};  // horizon + fog color
    glm::vec3 groundColor{0.10f, 0.09f, 0.08f};   // ambient lower hemisphere
    float fogDensity = 0.010f;
    float ambientIntensity = 1.0f;
};

enum class ETonemapper : uint8_t {
    kAces,
    kReinhard,
    kLinear,
};

struct PostProcessDesc_t {
    float exposure = 1.0f;
    ETonemapper tonemapper = ETonemapper::kAces;
    float bloomStrength = 0.35f;  // 0 = bloom off
    float bloomThreshold = 1.0f;
    float vignette = 0.4f;        // 0 = off
};

enum class ELightType : uint8_t {
    kDirectional,  // extra shadowless directional light (the sun is in EnvironmentDesc_t)
    kPoint,
    kSpot,
};

struct LightDesc_t {
    ELightType type = ELightType::kPoint;
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};  // direction the light travels (kDirectional, kSpot)
    glm::vec3 color{1.0f};                   // linear
    float intensity = 1.0f;
    float range = 10.0f;                     // kPoint/kSpot: radius of influence (meters)
    float innerConeRadians = 0.35f;          // kSpot
    float outerConeRadians = 0.60f;          // kSpot
};

struct DrawItem_t {
    MeshHandle_t mesh;
    MaterialHandle_t material;  // if invalid: defaultMaterial()
    glm::mat4 transform{1.0f};
    bool castShadow = true;
    // Skin matrices for a kPosNormalUvSkin mesh (anim::evaluate output), at most kMaxBones. submit() copies them;
    // they are not kept after the call. If empty on a skinned mesh, it is drawn in bind pose (identity matrices).
    std::span<const glm::mat4> bones;
};

}  // namespace game
