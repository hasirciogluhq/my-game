#include "engine/ui/rml/actions.hpp"

#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>

namespace game::rml {

namespace {

// Bir öznitelik için yaratılır; RmlUi OnDetach'te sahipliği bırakır (bu nesne
// kendini siler).
class CActionListener final : public Rml::EventListener {
public:
  CActionListener(const Rml::String &attribute, std::deque<UiAction_t> &queue)
      : m_queue(queue) {
    const size_t colon = attribute.find(':');
    m_name = attribute.substr(0, colon);
    if (colon != Rml::String::npos) {
      m_argument = attribute.substr(colon + 1);
    }
  }

  void ProcessEvent(Rml::Event &event) override {
    // change olayında elemanın yeni değeri parametre olarak gelir; yoksa
    // özniteliğin ":arg" kısmı kullanılır.
    const Rml::String value =
        event.GetParameter<Rml::String>("value", m_argument);
    m_queue.push_back({.name = m_name, .value = value});
  }

  void OnDetach(Rml::Element *) override {
    delete this;
  } // RmlUi dinleyici yaşam döngüsü sözleşmesi: nesne kendini siler

private:
  std::deque<UiAction_t> &m_queue;
  Rml::String m_name;
  Rml::String m_argument;
};

} // namespace

Rml::EventListener *CRmlActions::InstanceEventListener(const Rml::String &value,
                                                       Rml::Element *) {
  return new CActionListener(
      value, m_queue); // RmlUi API'si ham işaretçi ister; OnDetach siler
}

std::optional<UiAction_t> CRmlActions::next() {
  if (m_queue.empty()) {
    return std::nullopt;
  }
  UiAction_t action = std::move(m_queue.front());
  m_queue.pop_front();
  return action;
}

} // namespace game::rml
