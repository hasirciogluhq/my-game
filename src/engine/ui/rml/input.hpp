#pragma once

#include <SDL3/SDL_events.h>

#include <RmlUi/Core/Context.h>

namespace game::rml::input {

// Converts an SDL event to the RmlUi context. pixelDensity: draw pixels per window point
// (the mouse position is converted to pixels). If mouse/keyboard is false, the related
// events are ignored (e.g. ImGui wants that input).
void process(Rml::Context &context, const SDL_Event &event, float pixelDensity,
             bool mouse, bool keyboard);

} // namespace game::rml::input
