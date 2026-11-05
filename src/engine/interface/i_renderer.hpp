#pragma once

#include "engine/interface/render_types.hpp"

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace game {

// ---------------------------------------------------------------------------
// Renderer001: backend'den bağımsız renderer (tek renderer arayüzü). Tipler interface/render_types.hpp'de.
//
// Kullanım sırası:
//   initialize -> createMesh/createTexture/createMaterial (yükleme zamanı, istendiğinde de olur)
//   -> her frame: beginFrame -> setView -> setEnvironment/setPostProcess -> submit* / submitLight* -> endFrame
//   -> shutdown
// Tek thread'den çağrılır. Draw/ışık/debug çizgileri endFrame'e kadar kaydedilir; endFrame hepsini çizer.
// Geçersiz veya silinmiş handle'lar sessizce yok sayılır (submit'te çizilmez).
// ---------------------------------------------------------------------------
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // --- yaşam döngüsü ---
    [[nodiscard]] virtual bool initialize(const RendererDesc_t& desc) = 0;
    virtual void shutdown() = 0;
    [[nodiscard]] virtual const char* lastError() const = 0;
    [[nodiscard]] virtual const RenderCaps_t& capabilities() const = 0;

    // Çalışma anında değişebilir: vsync, gölge kalitesi/menzili, render ölçeği.
    virtual void applySettings(const RenderSettings_t& settings) = 0;
    [[nodiscard]] virtual const RenderSettings_t& settings() const = 0;

    // --- kaynaklar (geçersiz handle döner = hata, lastError()'a bakın) ---
    [[nodiscard]] virtual MeshHandle_t createMesh(const MeshDesc_t& desc) = 0;
    virtual void destroyMesh(MeshHandle_t mesh) = 0;

    [[nodiscard]] virtual TextureHandle_t createTexture(const TextureDesc_t& desc) = 0;
    virtual void destroyTexture(TextureHandle_t texture) = 0;

    [[nodiscard]] virtual MaterialHandle_t createMaterial(const MaterialDesc_t& desc) = 0;
    virtual void updateMaterial(MaterialHandle_t material, const MaterialDesc_t& desc) = 0;
    virtual void destroyMaterial(MaterialHandle_t material) = 0;
    // Beyaz, kLit, opak. Silinemez.
    [[nodiscard]] virtual MaterialHandle_t defaultMaterial() const = 0;

    // --- frame ---
    virtual void beginFrame(const FrameDesc_t& frame) = 0;
    virtual void setView(const ViewDesc_t& view) = 0;
    virtual void setEnvironment(const EnvironmentDesc_t& environment) = 0;
    virtual void setPostProcess(const PostProcessDesc_t& post) = 0;

    virtual void submit(const DrawItem_t& item) = 0;
    virtual void submitLight(const LightDesc_t& light) = 0;
    // Derinlik testli, dünya uzayında çizgi (HDR sahne üstüne). color sRGB-benzeri ham değer.
    virtual void submitDebugLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) = 0;

    // Overlay (Dear ImGui): her frame post-process'ten sonra swapchain'e çizilen son adım. Kalıcıdır; nullptr kaldırır.
    // Renderer ne çizildiğini bilmez, backend'in komut tamponunu verir (bkz. RenderOverlayFn).
    virtual void setOverlay(RenderOverlayFn overlay, void* user) = 0;
    // Backend'in yerel aygıtı (SDL_GPU: SDL_GPUDevice*); initialize'dan önce nullptr. Dış backend'ler (ImGui) için.
    [[nodiscard]] virtual void* nativeDevice() const = 0;

    virtual void endFrame() = 0;

    [[nodiscard]] virtual const RenderStats_t& stats() const = 0;
};

inline constexpr char kRendererInterface[] = "Renderer001";

}  // namespace game
