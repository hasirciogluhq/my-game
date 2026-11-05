#pragma once

#include "engine/interface/render_types.hpp"

#include <optional>
#include <string>

namespace game {

// An action coming from an element's event attribute in RmlUi, such as onclick="..." / onchange="...".
// Written as "name" or "name:arg"; if arg is present it is the value. For change events the value is the element's value.
struct UiAction_t {
  std::string name;
  std::string value;
};

// ui.dll interface: the engine's two UI systems in one place.
//   1) RmlUi: the interface the player sees (menus, HUD, scoreboard). HTML/CSS-like .rml/.rcss files (assets/ui/).
//      RmlUi types do not leak out of this interface: documents are referred to by name (assets-relative path, "ui/hud.rml").
//   2) Dear ImGui: developer tools (console, debug). Game code writes ImGui directly: client.dll also links imgui,
//      and shares ui.dll's context through context() and the allocator.
//
// Frame flow: platform events -> processEvent (set with setEventHook) -> beginFrame -> ImGui::* and RmlUi calls -> endFrame.
// The renderer does the drawing: renderer->setOverlay(ui->overlay(), ui->overlayUser()) is called once.
// Overlay order: RmlUi (bottom), ImGui (top).
class IUI {
public:
  virtual ~IUI() = default;

  // sdlWindow: SDL_Window*, sdlGpuDevice: SDL_GPUDevice* (IRenderer::nativeDevice). Called after the renderer is initialized.
  // If RmlUi fails to initialize, ImGui still works (rmlAvailable() returns false).
  [[nodiscard]] virtual bool initialize(void *sdlWindow,
                                        void *sdlGpuDevice) = 0;
  virtual void shutdown() = 0;

  // event: const SDL_Event* (IPlatform::setEventHook gives this signature).
  virtual void processEvent(const void *event) = 0;
  virtual void beginFrame() = 0;
  virtual void endFrame() = 0;

  // Does ImGui want the keyboard/mouse? (to stop game input)
  [[nodiscard]] virtual bool wantsKeyboard() const = 0;
  [[nodiscard]] virtual bool wantsMouse() const = 0;

  [[nodiscard]] virtual RenderOverlayFn overlay() const = 0;
  [[nodiscard]] virtual void *overlayUser() = 0;

  // Raw values for ImGui::SetCurrentContext(context) + ImGui::SetAllocatorFunctions(alloc,
  // free, allocUser) (ImGui types do not leak out).
  [[nodiscard]] virtual void *context() const = 0;
  virtual void allocatorFunctions(void **alloc, void **free,
                                  void **user) const = 0;

  // --- RmlUi
  // ---------------------------------------------------------------------------------------------------
  // Design resolution is 1920x1080: 1dp = window height / 1080 pixels.
  // Use dp in .rcss.
  [[nodiscard]] virtual bool rmlAvailable() const = 0;

  // Loads TTF/OTF (assets-relative: "fonts/DejaVuSans.ttf"). If fallback is true, a backup face is used for missing glyphs.
  [[nodiscard]] virtual bool loadFont(const char *assetPath, bool fallback) = 0;

  // Loads a document by assets-relative path such as "ui/hud.rml"; it starts hidden. Returns true if the same name is already loaded.
  [[nodiscard]] virtual bool loadDocument(const char *name) = 0;
  virtual void unloadDocument(const char *name) = 0;
  // capturesInput true: while the document is visible, mouse/keyboard go to it and other documents get no input (menu).
  // false: display only (HUD).
  virtual void showDocument(const char *name, bool capturesInput) = 0;
  virtual void hideDocument(const char *name) = 0;
  [[nodiscard]] virtual bool isDocumentVisible(const char *name) const = 0;
  // Is there a visible document that captures input? Used to stop game input and free the mouse.
  [[nodiscard]] virtual bool rmlCapturesInput() const = 0;

  // Changes an element's content and appearance (id: the id attribute in the RML).
  // If the value is unchanged it does nothing; it can be called every frame.
  virtual void setElementRml(const char *document, const char *elementId,
                             const char *rml) = 0;
  virtual void setElementClass(const char *document, const char *elementId,
                               const char *className, bool enabled) = 0;
  virtual void setElementStyle(const char *document, const char *elementId,
                               const char *property, const char *value) = 0;

  // Actions of elements clicked/changed in recent frames, in order. Call until it is empty.
  [[nodiscard]] virtual std::optional<UiAction_t> nextAction() = 0;

  // RmlUi developer inspector (element tree, styles). Only active in debug builds.
  virtual void setDebugger(bool visible) = 0;
};

inline constexpr char kUIInterface[] = "UI001";

} // namespace game
