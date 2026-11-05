#pragma once

#include <cstdint>
#include <string>

#include <glm/vec2.hpp>

namespace game {

// event: const SDL_Event* (void* çünkü arayüz SDL'e bağımlı olmaz).
using PlatformEventFn = void (*)(const void* event, void* user);

struct PlatformEvents_t {
    bool quit = false;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
};

// platform.dll arayüzü: pencere, olay pompası, tuş durumu, metin girişi. SDL'e tek erişim noktası.
// Tuşlar isimle sorgulanır (yeniden atanabilir input sistemi için). Tuş adları SDL adlarıdır:
// "W", "Space", "Left Shift", "`", "F1"... Fare: "Mouse1", "Mouse2", "Mouse3".
class IPlatform {
public:
    virtual ~IPlatform() = default;

    [[nodiscard]] virtual bool createWindow(const char* title, int width, int height) = 0;
    virtual void destroyWindow() = 0;
    [[nodiscard]] virtual void* nativeWindow() const = 0;
    [[nodiscard]] virtual glm::ivec2 drawableSize() const = 0;

    [[nodiscard]] virtual PlatformEvents_t pollEvents() = 0;

    // Fare konumu, drawableSize() ile aynı piksel uzayında (sol üst orijin).
    [[nodiscard]] virtual glm::vec2 mousePosition() const = 0;
    // Pencere pikseli başına çizim pikseli (retina = 2). UI boyutu ve fare konumu buna göre ölçeklenir.
    [[nodiscard]] virtual float pixelDensity() const = 0;

    virtual void setTitle(const char* title) = 0;
    virtual void setRelativeMouse(bool enabled) = 0;

    [[nodiscard]] virtual bool keyDownByName(const char* keyName) const = 0;
    // Son pollEvents() sırasında yeni basıldı mı? (tuş tekrarı sayılmaz)
    [[nodiscard]] virtual bool keyPressedByName(const char* keyName) const = 0;
    // Her SDL_Event için pollEvents() içinde çağrılır (event: const SDL_Event*). Dear ImGui SDL3 backend'i buradan beslenir.
    virtual void setEventHook(PlatformEventFn hook, void* user) = 0;
};

inline constexpr char kPlatformInterface[] = "Platform001";

}  // namespace game
