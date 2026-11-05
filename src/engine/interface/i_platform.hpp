#pragma once

#include <cstdint>
#include <string>

#include <glm/vec2.hpp>

namespace game {

// event: const SDL_Event* (void* because the interface does not depend on SDL).
using PlatformEventFn = void (*)(const void* event, void* user);

struct PlatformEvents_t {
    bool quit = false;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
};

// platform.dll interface: window, event pump, key state, text input. The single access point to SDL.
// Keys are queried by name (for the rebindable input system). Key names are SDL names:
// "W", "Space", "Left Shift", "`", "F1"... Mouse: "Mouse1", "Mouse2", "Mouse3".
class IPlatform {
public:
    virtual ~IPlatform() = default;

    [[nodiscard]] virtual bool createWindow(const char* title, int width, int height) = 0;
    virtual void destroyWindow() = 0;
    [[nodiscard]] virtual void* nativeWindow() const = 0;
    [[nodiscard]] virtual glm::ivec2 drawableSize() const = 0;

    [[nodiscard]] virtual PlatformEvents_t pollEvents() = 0;

    // Mouse position, in the same pixel space as drawableSize() (top-left origin).
    [[nodiscard]] virtual glm::vec2 mousePosition() const = 0;
    // Draw pixels per window pixel (retina = 2). UI size and mouse position are scaled accordingly.
    [[nodiscard]] virtual float pixelDensity() const = 0;

    virtual void setTitle(const char* title) = 0;
    virtual void setRelativeMouse(bool enabled) = 0;

    [[nodiscard]] virtual bool keyDownByName(const char* keyName) const = 0;
    // Was it newly pressed during the last pollEvents()? (key repeat is not counted)
    [[nodiscard]] virtual bool keyPressedByName(const char* keyName) const = 0;
    // Called for every SDL_Event inside pollEvents() (event: const SDL_Event*). Feeds the Dear ImGui SDL3 backend.
    virtual void setEventHook(PlatformEventFn hook, void* user) = 0;
};

inline constexpr char kPlatformInterface[] = "Platform001";

}  // namespace game
