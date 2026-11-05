#include "engine/ui/rml/rml_system.hpp"

#include <algorithm>

#include <SDL3/SDL.h>

#include <RmlUi/Debugger.h>

#include "engine/interface/i_log.hpp"
#include "engine/ui/rml/input.hpp"

namespace game::rml {

namespace {

constexpr float kDesignHeight =
    1080.0f; // 1dp = pencere yüksekliği / 1080 piksel

} // namespace

bool CRmlSystem::initialize(SDL_Window *window, SDL_GPUDevice *device) {
  m_window = window;
  if (!m_renderer.initialize(device, window)) {
    return false;
  }
  Rml::SetSystemInterface(&m_systemInterface);
  Rml::SetFileInterface(&m_fileInterface);
  Rml::SetRenderInterface(&m_renderer);
  if (!Rml::Initialise()) {
    GAME_LOG_ERROR("ui", "RmlUi başlatılamadı");
    m_renderer.shutdown();
    return false;
  }
  Rml::Factory::RegisterEventListenerInstancer(&m_actions);
  m_context = Rml::CreateContext("game", Rml::Vector2i{1280, 720});
  if (m_context == nullptr) {
    GAME_LOG_ERROR("ui", "RmlUi bağlamı oluşturulamadı");
    Rml::Shutdown();
    m_renderer.shutdown();
    return false;
  }
#ifndef NDEBUG
  m_debuggerReady = Rml::Debugger::Initialise(m_context);
  Rml::Debugger::SetVisible(false);
#endif
  syncViewport();
  return true;
}

void CRmlSystem::shutdown() {
  if (m_context == nullptr) {
    return;
  }
  if (m_textInputActive) {
    SDL_StopTextInput(m_window);
  }
  m_documents.clear();
  m_lastRml.clear();
  m_actions.clear();
  m_context = nullptr;
  Rml::Shutdown(); // bağlamları ve belgeleri yok eder; render/sistem arayüzleri
                   // bundan sonra serbest kalır
  m_renderer.shutdown();
}

// --- girdi
// ---------------------------------------------------------------------------------------------------------

void CRmlSystem::processEvent(const SDL_Event &event, bool mouseAllowed,
                              bool keyboardAllowed) {
  if (m_context == nullptr || !capturesInput()) {
    return;
  }
  input::process(*m_context, event, m_pixelDensity, mouseAllowed,
                 keyboardAllowed);
}

// --- frame
// ---------------------------------------------------------------------------------------------------------

void CRmlSystem::syncViewport() {
  int width = 0;
  int height = 0;
  SDL_GetWindowSizeInPixels(m_window, &width, &height);
  m_pixelDensity = SDL_GetWindowPixelDensity(m_window);
  const glm::ivec2 size{std::max(width, 1), std::max(height, 1)};
  if (size == m_viewport) {
    return;
  }
  m_viewport = size;
  m_context->SetDimensions({size.x, size.y});
  m_context->SetDensityIndependentPixelRatio(static_cast<float>(size.y) /
                                             kDesignHeight);
}

void CRmlSystem::update() {
  if (m_context == nullptr) {
    return;
  }
  // Girdi alan belge varken metin girişi açık olmalı (input kutuları için
  // SDL_EVENT_TEXT_INPUT).
  const bool wantsText = capturesInput();
  if (wantsText != m_textInputActive) {
    m_textInputActive = wantsText;
    if (wantsText) {
      SDL_StartTextInput(m_window);
    } else {
      SDL_StopTextInput(m_window);
    }
  }
  syncViewport();
  m_context->Update();
  m_renderer.beginFrame(m_viewport);
  m_context->Render();
}

// --- belgeler
// ------------------------------------------------------------------------------------------------------

bool CRmlSystem::loadFont(const std::string &assetPath, bool fallback) {
  return Rml::LoadFontFace(assetPath, fallback);
}

bool CRmlSystem::loadDocument(const std::string &name) {
  if (m_context == nullptr) {
    return false;
  }
  if (m_documents.contains(name)) {
    return true;
  }
  Rml::ElementDocument *document = m_context->LoadDocument(name);
  if (document == nullptr) {
    GAME_LOG_ERROR("ui", "belge yüklenemedi: {}", name);
    return false;
  }
  document->Hide(); // gizli başlar; showDocument açar
  m_documents.emplace(name, Document_t{.document = document});
  return true;
}

void CRmlSystem::unloadDocument(const std::string &name) {
  const auto it = m_documents.find(name);
  if (it == m_documents.end()) {
    return;
  }
  it->second.document->Close();
  m_documents.erase(it);
  std::erase_if(m_lastRml, [&](const auto &entry) {
    return entry.first.starts_with(name + "#");
  });
}

void CRmlSystem::showDocument(const std::string &name, bool capturesInput) {
  const auto it = m_documents.find(name);
  if (it == m_documents.end()) {
    return;
  }
  it->second.capturesInput = capturesInput;
  if (capturesInput) {
    it->second.document->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Auto);
  } else {
    it->second.document->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
  }
}

void CRmlSystem::hideDocument(const std::string &name) {
  const auto it = m_documents.find(name);
  if (it != m_documents.end()) {
    it->second.document->Hide();
  }
}

bool CRmlSystem::isVisible(const std::string &name) const {
  const auto it = m_documents.find(name);
  return it != m_documents.end() && it->second.document->IsVisible();
}

bool CRmlSystem::capturesInput() const {
  return std::ranges::any_of(m_documents, [](const auto &entry) {
    return entry.second.capturesInput && entry.second.document->IsVisible();
  });
}

// --- eleman değişiklikleri
// -----------------------------------------------------------------------------------------

Rml::Element *CRmlSystem::findElement(const std::string &document,
                                      const std::string &id) const {
  const auto it = m_documents.find(document);
  return it == m_documents.end() ? nullptr
                                 : it->second.document->GetElementById(id);
}

void CRmlSystem::setElementRml(const std::string &document,
                               const std::string &id, const std::string &rml) {
  Rml::Element *element = findElement(document, id);
  if (element == nullptr) {
    return;
  }
  std::string &last = m_lastRml[document + "#" + id];
  if (last == rml) {
    return;
  }
  last = rml;
  element->SetInnerRML(rml);
}

void CRmlSystem::setElementClass(const std::string &document,
                                 const std::string &id,
                                 const std::string &className, bool enabled) {
  if (Rml::Element *element = findElement(document, id)) {
    element->SetClass(className, enabled);
  }
}

void CRmlSystem::setElementStyle(const std::string &document,
                                 const std::string &id,
                                 const std::string &property,
                                 const std::string &value) {
  if (Rml::Element *element = findElement(document, id)) {
    element->SetProperty(property, value);
  }
}

void CRmlSystem::setDebugger(bool visible) {
  if (m_debuggerReady) {
    Rml::Debugger::SetVisible(visible);
  }
}

} // namespace game::rml
