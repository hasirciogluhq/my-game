#include "engine/ui/rml/input.hpp"

#include <array>
#include <utility>

#include <RmlUi/Core/Input.h>

namespace game::rml::input {

namespace {

using Key = Rml::Input::KeyIdentifier;

constexpr std::array<std::pair<SDL_Scancode, Key>, 23> kSpecialKeys{{
    {SDL_SCANCODE_SPACE, Key::KI_SPACE},
    {SDL_SCANCODE_RETURN, Key::KI_RETURN},
    {SDL_SCANCODE_KP_ENTER, Key::KI_NUMPADENTER},
    {SDL_SCANCODE_ESCAPE, Key::KI_ESCAPE},
    {SDL_SCANCODE_BACKSPACE, Key::KI_BACK},
    {SDL_SCANCODE_TAB, Key::KI_TAB},
    {SDL_SCANCODE_DELETE, Key::KI_DELETE},
    {SDL_SCANCODE_HOME, Key::KI_HOME},
    {SDL_SCANCODE_END, Key::KI_END},
    {SDL_SCANCODE_PAGEUP, Key::KI_PRIOR},
    {SDL_SCANCODE_PAGEDOWN, Key::KI_NEXT},
    {SDL_SCANCODE_LEFT, Key::KI_LEFT},
    {SDL_SCANCODE_RIGHT, Key::KI_RIGHT},
    {SDL_SCANCODE_UP, Key::KI_UP},
    {SDL_SCANCODE_DOWN, Key::KI_DOWN},
    {SDL_SCANCODE_LSHIFT, Key::KI_LSHIFT},
    {SDL_SCANCODE_RSHIFT, Key::KI_RSHIFT},
    {SDL_SCANCODE_LCTRL, Key::KI_LCONTROL},
    {SDL_SCANCODE_RCTRL, Key::KI_RCONTROL},
    {SDL_SCANCODE_LALT, Key::KI_LMENU},
    {SDL_SCANCODE_RALT, Key::KI_RMENU},
    {SDL_SCANCODE_MINUS, Key::KI_OEM_MINUS},
    {SDL_SCANCODE_PERIOD, Key::KI_OEM_PERIOD},
}};

// Harfler, rakamlar, F tuşları ve özel tuşlar. Tanımsız tuş: KI_UNKNOWN.
Key toKey(SDL_Scancode scancode) {
  if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
    return static_cast<Key>(Key::KI_A + (scancode - SDL_SCANCODE_A));
  }
  if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
    return static_cast<Key>(Key::KI_1 + (scancode - SDL_SCANCODE_1));
  }
  if (scancode == SDL_SCANCODE_0) {
    return Key::KI_0;
  }
  if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
    return static_cast<Key>(Key::KI_F1 + (scancode - SDL_SCANCODE_F1));
  }
  for (const auto &[sdl, rml] : kSpecialKeys) {
    if (sdl == scancode) {
      return rml;
    }
  }
  return Key::KI_UNKNOWN;
}

int modifiers() {
  const SDL_Keymod state = SDL_GetModState();
  int result = 0;
  result |= (state & SDL_KMOD_CTRL) ? Rml::Input::KM_CTRL : 0;
  result |= (state & SDL_KMOD_SHIFT) ? Rml::Input::KM_SHIFT : 0;
  result |= (state & SDL_KMOD_ALT) ? Rml::Input::KM_ALT : 0;
  result |= (state & SDL_KMOD_GUI) ? Rml::Input::KM_META : 0;
  return result;
}

// SDL: 1 sol, 2 orta, 3 sağ. RmlUi: 0 sol, 1 sağ, 2 orta. Tanımsız: -1.
int toButton(Uint8 sdlButton) {
  switch (sdlButton) {
  case SDL_BUTTON_LEFT:
    return 0;
  case SDL_BUTTON_RIGHT:
    return 1;
  case SDL_BUTTON_MIDDLE:
    return 2;
  default:
    return -1;
  }
}

} // namespace

void process(Rml::Context &context, const SDL_Event &event, float pixelDensity,
             bool mouse, bool keyboard) {
  switch (event.type) {
  case SDL_EVENT_MOUSE_MOTION:
    if (mouse) {
      context.ProcessMouseMove(static_cast<int>(event.motion.x * pixelDensity),
                               static_cast<int>(event.motion.y * pixelDensity),
                               modifiers());
    }
    break;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (mouse && toButton(event.button.button) >= 0) {
      if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        context.ProcessMouseButtonDown(toButton(event.button.button),
                                       modifiers());
      } else {
        context.ProcessMouseButtonUp(toButton(event.button.button),
                                     modifiers());
      }
    }
    break;
  case SDL_EVENT_MOUSE_WHEEL:
    if (mouse) {
      context.ProcessMouseWheel(Rml::Vector2f{-event.wheel.x, -event.wheel.y},
                                modifiers());
    }
    break;
  case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    context.ProcessMouseLeave();
    break;
  case SDL_EVENT_KEY_DOWN:
    if (keyboard) {
      context.ProcessKeyDown(toKey(event.key.scancode), modifiers());
    }
    break;
  case SDL_EVENT_KEY_UP:
    if (keyboard) {
      context.ProcessKeyUp(toKey(event.key.scancode), modifiers());
    }
    break;
  case SDL_EVENT_TEXT_INPUT:
    if (keyboard) {
      context.ProcessTextInput(Rml::String{event.text.text});
    }
    break;
  default:
    break;
  }
}

} // namespace game::rml::input
