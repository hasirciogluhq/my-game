#pragma once

#include <deque>
#include <optional>

#include <RmlUi/Core/EventListenerInstancer.h>

#include "engine/interface/i_ui.hpp"

namespace game::rml {

// The onclick="name" / onclick="name:arg" / onchange="name" attributes in RML reach the game
// as actions. Game code does not run inside the RmlUi event handler: actions are queued,
// and the game reads them with nextAction().
class CRmlActions final : public Rml::EventListenerInstancer {
public:
  Rml::EventListener *InstanceEventListener(const Rml::String &value,
                                            Rml::Element *element) override;

  [[nodiscard]] std::optional<UiAction_t> next();
  void clear() { m_queue.clear(); }

private:
  std::deque<UiAction_t> m_queue;
};

} // namespace game::rml
