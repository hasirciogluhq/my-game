#pragma once

#include <deque>
#include <optional>

#include <RmlUi/Core/EventListenerInstancer.h>

#include "engine/interface/i_ui.hpp"

namespace game::rml {

// RML'deki onclick="ad" / onclick="ad:arg" / onchange="ad" öznitelikleri oyuna
// eylem olarak ulaşır. Oyun kodu RmlUi olay işleyicisinin içinde çalışmaz:
// eylemler kuyruğa yazılır, oyun nextAction() ile okur.
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
