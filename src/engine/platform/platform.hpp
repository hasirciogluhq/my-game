#pragma once

#include "engine/interface/i_platform.hpp"

#include <bitset>
#include <string>

struct SDL_Window;

namespace game {

// SDL3 window + events + input. All SDL access lives in this module.
class CPlatform final : public IPlatform {
public:
    CPlatform() = default;
    CPlatform(const CPlatform&) = delete;
    CPlatform& operator=(const CPlatform&) = delete;
    ~CPlatform() override;

    [[nodiscard]] bool createWindow(const char* title, int width, int height) override;
    void destroyWindow() override;
    [[nodiscard]] void* nativeWindow() const override;
    [[nodiscard]] glm::ivec2 drawableSize() const override;

    [[nodiscard]] PlatformEvents_t pollEvents() override;

    void setTitle(const char* title) override;
    void setRelativeMouse(bool enabled) override;

    [[nodiscard]] bool keyDownByName(const char* keyName) const override;
    [[nodiscard]] bool keyPressedByName(const char* keyName) const override;
    [[nodiscard]] glm::vec2 mousePosition() const override;
    [[nodiscard]] float pixelDensity() const override;
    void setEventHook(PlatformEventFn hook, void* user) override;

private:
    SDL_Window* m_window = nullptr;
    // Keys pressed during the last pollEvents() (indexed by scancode).
    std::bitset<512> m_pressed;
    PlatformEventFn m_hook = nullptr;
    void* m_hookUser = nullptr;
};

}  // namespace game
