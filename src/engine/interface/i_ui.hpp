#pragma once

#include "engine/interface/render_types.hpp"

#include <optional>
#include <string>

namespace game {

// RmlUi'de bir elemanın onclick="..." / onchange="..." gibi olay özniteliğinden
// gelen eylem. "ad" ya da "ad:arg" yazılır; arg varsa value'dur. change
// olaylarında value, elemanın değeridir.
struct UiAction_t {
  std::string name;
  std::string value;
};

// ui.dll arayüzü: motorun iki UI sistemi tek yerde.
//   1) RmlUi: oyuncunun gördüğü arayüz (menü, HUD, skor tablosu). HTML/CSS
//   benzeri .rml/.rcss dosyaları (assets/ui/).
//      RmlUi tipleri bu arayüzden dışarı sızmaz: belgeler isimle (assets-göreli
//      yol, "ui/hud.rml") anılır.
//   2) Dear ImGui: geliştirici araçları (konsol, debug). Oyun ImGui kodunu
//   doğrudan yazar: client.dll da imgui'yi bağlar,
//      context() ve allocator ile ui.dll'in bağlamını paylaşır.
//
// Frame akışı: platform olayları -> processEvent (setEventHook ile) ->
// beginFrame -> ImGui::* ve RmlUi çağrıları -> endFrame. Çizimi renderer yapar:
// renderer->setOverlay(ui->overlay(), ui->overlayUser()) bir kez çağrılır.
// Overlay sırası: RmlUi (alt), ImGui (üst).
class IUI {
public:
  virtual ~IUI() = default;

  // sdlWindow: SDL_Window*, sdlGpuDevice: SDL_GPUDevice*
  // (IRenderer::nativeDevice). Renderer başlatıldıktan sonra çağrılır. RmlUi
  // başlatılamazsa ImGui yine çalışır (rmlAvailable() false olur).
  [[nodiscard]] virtual bool initialize(void *sdlWindow,
                                        void *sdlGpuDevice) = 0;
  virtual void shutdown() = 0;

  // event: const SDL_Event* (IPlatform::setEventHook bu imzayı verir).
  virtual void processEvent(const void *event) = 0;
  virtual void beginFrame() = 0;
  virtual void endFrame() = 0;

  // ImGui klavye/fareyi istiyor mu? (oyun girdisini durdurmak için)
  [[nodiscard]] virtual bool wantsKeyboard() const = 0;
  [[nodiscard]] virtual bool wantsMouse() const = 0;

  [[nodiscard]] virtual RenderOverlayFn overlay() const = 0;
  [[nodiscard]] virtual void *overlayUser() = 0;

  // ImGui::SetCurrentContext(context) + ImGui::SetAllocatorFunctions(alloc,
  // free, allocUser) için ham değerler (imgui türleri sızmaz).
  [[nodiscard]] virtual void *context() const = 0;
  virtual void allocatorFunctions(void **alloc, void **free,
                                  void **user) const = 0;

  // --- RmlUi
  // ---------------------------------------------------------------------------------------------------
  // Tasarım çözünürlüğü 1920x1080'dir: 1dp = pencere yüksekliği / 1080 piksel.
  // .rcss'te dp kullanın.
  [[nodiscard]] virtual bool rmlAvailable() const = 0;

  // TTF/OTF yükler (assets-göreli: "fonts/DejaVuSans.ttf"). fallback true ise
  // eksik harfler için yedek yüz olur.
  [[nodiscard]] virtual bool loadFont(const char *assetPath, bool fallback) = 0;

  // "ui/hud.rml" gibi assets-göreli yolla belge yükler; gizli başlar. Aynı isim
  // zaten yüklüyse true döner.
  [[nodiscard]] virtual bool loadDocument(const char *name) = 0;
  virtual void unloadDocument(const char *name) = 0;
  // capturesInput true: belge görünürken fare/klavye ona gider ve diğer
  // belgeler girdi almaz (menü). false: salt görüntü (HUD).
  virtual void showDocument(const char *name, bool capturesInput) = 0;
  virtual void hideDocument(const char *name) = 0;
  [[nodiscard]] virtual bool isDocumentVisible(const char *name) const = 0;
  // Girdi alan (capturesInput) görünür bir belge var mı? Oyun girdisini
  // durdurmak ve fareyi serbest bırakmak için.
  [[nodiscard]] virtual bool rmlCapturesInput() const = 0;

  // Eleman içeriğini ve görünümünü değiştirir (id: RML'deki id özniteliği).
  // Değer aynıysa iş yapmaz, her frame çağrılabilir.
  virtual void setElementRml(const char *document, const char *elementId,
                             const char *rml) = 0;
  virtual void setElementClass(const char *document, const char *elementId,
                               const char *className, bool enabled) = 0;
  virtual void setElementStyle(const char *document, const char *elementId,
                               const char *property, const char *value) = 0;

  // Son frame'lerde tıklanan/değişen elemanların eylemleri, sırayla. Boşalana
  // kadar çağırın.
  [[nodiscard]] virtual std::optional<UiAction_t> nextAction() = 0;

  // RmlUi geliştirici denetçisi (eleman ağacı, stiller). Sadece debug derlemede
  // etkindir.
  virtual void setDebugger(bool visible) = 0;
};

inline constexpr char kUIInterface[] = "UI001";

} // namespace game
