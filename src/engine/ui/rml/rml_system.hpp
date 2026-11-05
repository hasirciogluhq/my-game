#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <glm/vec2.hpp>

#include <RmlUi/Core.h>

#include "engine/interface/i_ui.hpp"
#include "engine/ui/rml/actions.hpp"
#include "engine/ui/rml/host.hpp"
#include "engine/ui/rml/render.hpp"

struct SDL_GPUCommandBuffer;
struct SDL_GPUDevice;
struct SDL_GPURenderPass;
struct SDL_Window;
union SDL_Event;

namespace game::rml {

// RmlUi'nin ui.dll içindeki sahibi: başlatma, bağlam, belgeler, eylem kuyruğu,
// girdi yönlendirme, çizim. Dışarıya RmlUi tipi vermez; IUI üzerinden isimle
// konuşulur.
class CRmlSystem {
public:
  CRmlSystem() = default;
  CRmlSystem(const CRmlSystem &) = delete;
  CRmlSystem &operator=(const CRmlSystem &) = delete;

  [[nodiscard]] bool initialize(SDL_Window *window, SDL_GPUDevice *device);
  void shutdown();
  [[nodiscard]] bool available() const { return m_context != nullptr; }

  void processEvent(const SDL_Event &event, bool mouseAllowed,
                    bool keyboardAllowed);
  // Frame sonu: boyutu eşitler, belgeleri günceller, çizim komutlarını
  // kaydeder.
  void update();
  void prepare(SDL_GPUCommandBuffer *commands) { m_renderer.prepare(commands); }
  void draw(SDL_GPUCommandBuffer *commands, SDL_GPURenderPass *pass) {
    m_renderer.draw(commands, pass);
  }

  [[nodiscard]] bool loadFont(const std::string &assetPath, bool fallback);
  [[nodiscard]] bool loadDocument(const std::string &name);
  void unloadDocument(const std::string &name);
  void showDocument(const std::string &name, bool capturesInput);
  void hideDocument(const std::string &name);
  [[nodiscard]] bool isVisible(const std::string &name) const;
  [[nodiscard]] bool capturesInput() const;

  void setElementRml(const std::string &document, const std::string &id,
                     const std::string &rml);
  void setElementClass(const std::string &document, const std::string &id,
                       const std::string &className, bool enabled);
  void setElementStyle(const std::string &document, const std::string &id,
                       const std::string &property, const std::string &value);

  [[nodiscard]] std::optional<UiAction_t> nextAction() {
    return m_actions.next();
  }
  void setDebugger(bool visible);

private:
  struct Document_t {
    Rml::ElementDocument *document = nullptr;
    bool capturesInput = false;
  };

  [[nodiscard]] Rml::Element *findElement(const std::string &document,
                                          const std::string &id) const;
  void syncViewport();

  SDL_Window *m_window = nullptr;
  CRmlSystemInterface m_systemInterface;
  CRmlFileInterface m_fileInterface;
  CRmlRenderer m_renderer;
  CRmlActions m_actions;

  Rml::Context *m_context = nullptr;
  std::unordered_map<std::string, Document_t> m_documents;
  std::unordered_map<std::string, std::string>
      m_lastRml; // "belge#id" -> son yazılan içerik (aynıysa yeniden yazma)
  glm::ivec2 m_viewport{0};
  float m_pixelDensity = 1.0f;
  bool m_debuggerReady = false;
  bool m_textInputActive = false;
};

} // namespace game::rml
