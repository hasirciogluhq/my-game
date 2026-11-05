#pragma once

#include "engine/interface/i_ui.hpp"
#include "engine/ui/api.hpp"
#include "engine/ui/rml/rml_system.hpp"

namespace game {

// UI001 implementation: RmlUi (game UI, SDL_GPU drawing in rml/render.cpp) + Dear
// ImGui (developer tools, the official SDL3 / SDL_GPU3 backends).
class UI_API CUiSystem final : public IUI {
public:
  CUiSystem() = default;
  CUiSystem(const CUiSystem &) = delete;
  CUiSystem &operator=(const CUiSystem &) = delete;
  ~CUiSystem() override;

  [[nodiscard]] bool initialize(void *sdlWindow, void *sdlGpuDevice) override;
  void shutdown() override;

  void processEvent(const void *event) override;
  void beginFrame() override;
  void endFrame() override;

  [[nodiscard]] bool wantsKeyboard() const override;
  [[nodiscard]] bool wantsMouse() const override;

  [[nodiscard]] RenderOverlayFn overlay() const override;
  [[nodiscard]] void *overlayUser() override { return this; }

  [[nodiscard]] void *context() const override;
  void allocatorFunctions(void **alloc, void **free,
                          void **user) const override;

  [[nodiscard]] bool rmlAvailable() const override { return m_rml.available(); }
  [[nodiscard]] bool loadFont(const char *assetPath, bool fallback) override {
    return m_rml.loadFont(assetPath, fallback);
  }
  [[nodiscard]] bool loadDocument(const char *name) override {
    return m_rml.loadDocument(name);
  }
  void unloadDocument(const char *name) override { m_rml.unloadDocument(name); }
  void showDocument(const char *name, bool capturesInput) override {
    m_rml.showDocument(name, capturesInput);
  }
  void hideDocument(const char *name) override { m_rml.hideDocument(name); }
  [[nodiscard]] bool isDocumentVisible(const char *name) const override {
    return m_rml.isVisible(name);
  }
  [[nodiscard]] bool rmlCapturesInput() const override {
    return m_rml.capturesInput();
  }
  void setElementRml(const char *document, const char *elementId,
                     const char *rml) override {
    m_rml.setElementRml(document, elementId, rml);
  }
  void setElementClass(const char *document, const char *elementId,
                       const char *className, bool enabled) override {
    m_rml.setElementClass(document, elementId, className, enabled);
  }
  void setElementStyle(const char *document, const char *elementId,
                       const char *property, const char *value) override {
    m_rml.setElementStyle(document, elementId, property, value);
  }
  [[nodiscard]] std::optional<UiAction_t> nextAction() override {
    return m_rml.nextAction();
  }
  void setDebugger(bool visible) override { m_rml.setDebugger(visible); }

  // Overlay gate for the renderer (called by the static drawOverlay).
  void recordOverlay(void *commandBuffer, void *renderPass);

private:
  bool m_initialized = false;
  rml::CRmlSystem m_rml;
};

} // namespace game
