#include "engine/platform/platform.hpp"

#include <SDL3/SDL.h>

#include "engine/interface/i_log.hpp"

#include <string_view>
#include <utility>

namespace game {

CPlatform::~CPlatform() {
    destroyWindow();
}

bool CPlatform::createWindow(const char* title, int width, int height) {
    if (m_window != nullptr) {
        return true;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        GAME_LOG_ERROR("platform", "SDL_Init başarısız: {}", SDL_GetError());
        return false;
    }
    m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (m_window == nullptr) {
        GAME_LOG_ERROR("platform", "SDL_CreateWindow başarısız: {}", SDL_GetError());
        SDL_Quit();
        return false;
    }
    return true;
}

void CPlatform::destroyWindow() {
    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        SDL_Quit();
    }
}

void* CPlatform::nativeWindow() const {
    return m_window;
}

glm::ivec2 CPlatform::drawableSize() const {
    int w = 0;
    int h = 0;
    if (m_window != nullptr) {
        SDL_GetWindowSizeInPixels(m_window, &w, &h);
    }
    return {w, h};
}

PlatformEvents_t CPlatform::pollEvents() {
    PlatformEvents_t events;
    m_pressed.reset();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (m_hook != nullptr) {
            m_hook(&event, m_hookUser);
        }
        switch (event.type) {
            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode < m_pressed.size()) {
                    if (!event.key.repeat) {
                        m_pressed.set(event.key.scancode);
                    }
                }
                break;
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                events.quit = true;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                events.mouseDeltaX += event.motion.xrel;
                events.mouseDeltaY += event.motion.yrel;
                break;
            default:
                break;
        }
    }
    return events;
}

namespace {

// "Mouse1".."Mouse3" fare tuşları, gerisi SDL tuş adı. Bilinmeyen ad: SDL_SCANCODE_UNKNOWN.
int mouseButtonFromName(std::string_view name) {
    if (name == "Mouse1") return SDL_BUTTON_LEFT;
    if (name == "Mouse2") return SDL_BUTTON_RIGHT;
    if (name == "Mouse3") return SDL_BUTTON_MIDDLE;
    return 0;
}

}  // namespace

bool CPlatform::keyDownByName(const char* keyName) const {
    if (const int button = mouseButtonFromName(keyName); button != 0) {
        return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(button)) != 0;
    }
    const SDL_Scancode scancode = SDL_GetScancodeFromName(keyName);
    return scancode != SDL_SCANCODE_UNKNOWN && SDL_GetKeyboardState(nullptr)[scancode];
}

bool CPlatform::keyPressedByName(const char* keyName) const {
    const SDL_Scancode scancode = SDL_GetScancodeFromName(keyName);
    return scancode != SDL_SCANCODE_UNKNOWN && m_pressed.test(scancode);
}

float CPlatform::pixelDensity() const {
    return m_window != nullptr ? SDL_GetWindowPixelDensity(m_window) : 1.0f;
}

glm::vec2 CPlatform::mousePosition() const {
    float x = 0.0f;
    float y = 0.0f;
    SDL_GetMouseState(&x, &y);
    return glm::vec2{x, y} * pixelDensity();
}

void CPlatform::setEventHook(PlatformEventFn hook, void* user) {
    m_hook = hook;
    m_hookUser = user;
}

void CPlatform::setTitle(const char* title) {
    if (m_window != nullptr) {
        SDL_SetWindowTitle(m_window, title);
    }
}

void CPlatform::setRelativeMouse(bool enabled) {
    if (m_window != nullptr) {
        SDL_SetWindowRelativeMouseMode(m_window, enabled);
    }
}

}  // namespace game
