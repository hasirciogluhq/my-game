#pragma once

// Renderer001 ortak tipleri. Backend'den (Metal / D3D11 / ...) tamamen bağımsızdır.
// Bu başlık sadece interface/ katmanındadır: engine'e veya render iç yapısına bağımlılığı yoktur.
//
// Konvansiyonlar (tüm backend'lerde aynı):
//   - Sağ-el koordinat sistemi, +Y yukarı, yaw=0 -Z'ye bakar.
//   - Projection derinliği 0..1 (glm::perspectiveRH_ZO). Matrisler glm column-major.
//   - Renk alanı: MaterialDesc_t::baseColor / emissive "sanatsal" sRGB girilir; renderer lineere çevirir.
//     Ortam/ışık renkleri (EnvironmentDesc_t, LightDesc_t) lineer uzaydadır.
//   - Işık yönleri "ışığın gittiği yön"dür (ışık kaynağından sahneye doğru).
//   - Birimler: metre, saniye, radyan.

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace game {

// ---------------------------------------------------------------------------
// Handle'lar: index + generation. generation == 0 geçersiz demektir (default-constructed = geçersiz).
// Silinmiş bir handle'ın yeniden kullanılması güvenlidir: eski handle çözülmez, çağrı yok sayılır.
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

inline constexpr uint32_t kMaxLights = 16;           // frame başına dinamik ışık (güneş hariç)
inline constexpr uint32_t kMaxDebugLines = 65536;    // frame başına debug çizgi üst sınırı

// ---------------------------------------------------------------------------
// Backend / yüzey
// ---------------------------------------------------------------------------
enum class ERenderBackend : uint8_t {
    kAuto,    // platformun varsayılanı (macOS: Metal)
    kMetal,
    kD3D11,   // henüz yok; initialize() hata döner
    kD3D12,
    kVulkan,
};

// Pencere native handle türü. Platform modülünün verdiği handle'ı tanımlar; renderer SDL'e bağımlı değildir.
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
    float shadowExtent = 45.0f;  // güneş gölgesinin kamera çevresinde kapladığı yarı genişlik (metre)
    float renderScale = 1.0f;    // iç çözünürlük çarpanı (0.25..2). Post pass ekrana ölçekler.
};

// Dışarıdan (Dear ImGui backend'i gibi) backend'in kendi komut tamponuna kayıt. Frame'de swapchain'e çizilen son adımdır.
// İki kez çağrılır: önce render pass dışında (nativeRenderPass == nullptr; GPU'ya veri yükleme), sonra pass içinde.
// SDL_GPU backend'inde: nativeCommandBuffer = SDL_GPUCommandBuffer*, nativeRenderPass = SDL_GPURenderPass*.
using RenderOverlayFn = void (*)(void* user, void* nativeCommandBuffer, void* nativeRenderPass);

struct RendererDesc_t {
    RenderSurface_t surface;
    ERenderBackend backend = ERenderBackend::kAuto;
    RenderSettings_t settings;
    bool debugLayer = false;  // backend doğrulama katmanı (yavaş)
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
    kPosNormalUv,  // float3 position, float3 normal, float2 uv = 32 byte. Position her layout'ta offset 0'dadır.
};

enum class EIndexType : uint8_t {
    kUint16,
    kUint32,
};

[[nodiscard]] constexpr uint32_t vertexStride(EVertexLayout layout) {
    switch (layout) {
        case EVertexLayout::kPosNormalUv: return 32;
    }
    return 0;
}

[[nodiscard]] constexpr uint32_t indexSize(EIndexType type) {
    return type == EIndexType::kUint16 ? 2u : 4u;
}

// Çağrı bitince renderer veriyi GPU'ya kopyalar; span'ler çağrıdan sonra tutulmaz.
struct MeshDesc_t {
    const char* debugName = "";
    EVertexLayout layout = EVertexLayout::kPosNormalUv;
    std::span<const std::byte> vertexData;
    EIndexType indexType = EIndexType::kUint32;
    std::span<const std::byte> indexData;
};

// Tipli vertex/index dizilerinden MeshDesc_t üretir. Örn: makeMeshDesc<Vertex_t, uint32_t>("box", v, i)
template <typename Vertex, typename Index>
[[nodiscard]] MeshDesc_t makeMeshDesc(const char* name, std::span<const Vertex> vertices, std::span<const Index> indices,
                                      EVertexLayout layout = EVertexLayout::kPosNormalUv) {
    static_assert(std::is_trivially_copyable_v<Vertex>);
    static_assert(sizeof(Index) == 2 || sizeof(Index) == 4, "index 16 veya 32 bit olmalı");
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
    kRgba8Unorm,      // lineer veri (normal map, maske...)
    kRgba8UnormSrgb,  // renk (albedo)
    kRgba16Float,
};

struct TextureDesc_t {
    const char* debugName = "";
    uint32_t width = 0;
    uint32_t height = 0;
    ETextureFormat format = ETextureFormat::kRgba8UnormSrgb;
    bool generateMips = true;
    std::span<const std::byte> pixels;  // mip 0, satırlar sıkı paketli (width * pikselBoyutu)
};

// ---------------------------------------------------------------------------
// Material
// ---------------------------------------------------------------------------
enum class EShadingModel : uint8_t {
    kLit,      // Blinn-Phong + güneş gölgesi + dinamik ışıklar + sis
    kRimLit,   // kLit + gövde gradyanı ve fresnel rim ışığı (karakterler için)
    kUnlit,    // sadece renk + emissive (sis uygulanır)
};

enum class EBlendMode : uint8_t {
    kOpaque,
    kAlphaBlend,  // derinlik yazmaz, arkadan öne sıralanır, gölge düşürmez
};

struct MaterialDesc_t {
    const char* debugName = "";
    EShadingModel shading = EShadingModel::kLit;
    EBlendMode blend = EBlendMode::kOpaque;
    glm::vec4 baseColor{1.0f};       // sRGB, a = opaklık (kAlphaBlend)
    TextureHandle_t baseColorTexture;  // geçersizse beyaz
    glm::vec3 emissive{0.0f};        // lineer, HDR olabilir
    float specularPower = 48.0f;
    float specularAmount = 0.18f;
    float rimAmount = 0.8f;          // sadece kRimLit
    float gridStrength = 0.0f;       // 0..1: dünya XZ düzleminde kareli desen (zemin için)
    bool doubleSided = true;
    bool castShadow = true;
};

// ---------------------------------------------------------------------------
// Frame / sahne
// ---------------------------------------------------------------------------
struct FrameDesc_t {
    float timeSeconds = 0.0f;
    float deltaSeconds = 0.0f;
};

struct ViewDesc_t {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};  // perspectiveRH_ZO
};

// Lineer renkler. sunDirection: güneş ışığının gittiği yön (normalize edilmesi gerekmez).
struct EnvironmentDesc_t {
    glm::vec3 sunDirection{-0.5f, -0.7f, -0.35f};
    glm::vec3 sunColor{1.0f, 0.93f, 0.8f};
    float sunIntensity = 3.2f;
    glm::vec3 skyColor{0.12f, 0.30f, 0.75f};      // zenit + ambient üst yarım küre
    glm::vec3 horizonColor{0.60f, 0.72f, 0.88f};  // ufuk + sis rengi
    glm::vec3 groundColor{0.10f, 0.09f, 0.08f};   // ambient alt yarım küre
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
    float bloomStrength = 0.35f;  // 0 = bloom kapalı
    float bloomThreshold = 1.0f;
    float vignette = 0.4f;        // 0 = kapalı
};

enum class ELightType : uint8_t {
    kDirectional,  // gölgesiz ek yön ışığı (güneş EnvironmentDesc_t'dedir)
    kPoint,
    kSpot,
};

struct LightDesc_t {
    ELightType type = ELightType::kPoint;
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};  // ışığın gittiği yön (kDirectional, kSpot)
    glm::vec3 color{1.0f};                   // lineer
    float intensity = 1.0f;
    float range = 10.0f;                     // kPoint/kSpot: etki yarıçapı (metre)
    float innerConeRadians = 0.35f;          // kSpot
    float outerConeRadians = 0.60f;          // kSpot
};

struct DrawItem_t {
    MeshHandle_t mesh;
    MaterialHandle_t material;  // geçersizse defaultMaterial()
    glm::mat4 transform{1.0f};
    bool castShadow = true;
};

}  // namespace game
