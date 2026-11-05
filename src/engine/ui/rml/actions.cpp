#include "engine/ui/rml/actions.hpp"

#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>

namespace game::rml {

namespace {

// Created for one attribute; RmlUi releases ownership in OnDetach (this object
// deletes itself).
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
    // On a change event the element's new value arrives as a parameter; otherwise
    // the ":arg" part of the attribute is used.
    const Rml::String value =
        event.GetParameter<Rml::String>("value", m_argument);
    m_queue.push_back({.name = m_name, .value = value});
  }

  void OnDetach(Rml::Element *) override {
    delete this;
  } // RmlUi listener lifecycle contract: the object deletes itself

private:
  std::deque<UiAction_t> &m_queue;
  Rml::String m_name;
  Rml::String m_argument;
};

} // namespace

Rml::EventListener *CRmlActions::InstanceEventListener(const Rml::String &value,
                                                       Rml::Element *) {
  return new CActionListener(
      value, m_queue); // the RmlUi API requires a raw pointer; OnDetach deletes it
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
