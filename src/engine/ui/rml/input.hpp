#pragma once

#include <SDL3/SDL_events.h>

#include <RmlUi/Core/Context.h>

namespace game::rml::input {

// SDL olayını RmlUi bağlamına çevirir. pixelDensity: pencere noktası başına
// çizim pikseli (fare konumu piksele çevrilir). mouse/keyboard false ise ilgili
// olaylar yok sayılır (ör. ImGui o girdiyi istiyor).
void process(Rml::Context &context, const SDL_Event &event, float pixelDensity,
             bool mouse, bool keyboard);

} // namespace game::rml::input
