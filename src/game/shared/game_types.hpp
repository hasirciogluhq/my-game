#pragma once

#include <cstddef>
#include <cstdint>

namespace game {

// ---------------------------------------------------------------------------
// Oyun sabitleri (birimler: metre, saniye)
// ---------------------------------------------------------------------------
inline constexpr float kRunSpeed = 6.0f;
inline constexpr float kWalkSpeed = 3.0f;
inline constexpr float kJumpSpeed = 6.0f;
inline constexpr float kGravity = 20.0f;
inline constexpr float kEyeHeight = 1.6f;
inline constexpr float kProjectileSpeed = 60.0f;
inline constexpr float kProjectileLifetime = 3.0f;
inline constexpr float kFireInterval = 0.1f;       // saniye, ateş hızı
inline constexpr uint32_t kProjectileDamage = 20;  // bir mermi isabetinde düşen can
inline constexpr size_t kMaxCmdBacklog = 3;        // server'da oyuncu başına bekleyen en çok komut

// ---------------------------------------------------------------------------
// Aksiyon butonları. Tuş değil, aksiyon: "attack" hangi tuştan geldiği fark etmez.
// ---------------------------------------------------------------------------
enum class EButton : uint32_t {
    kAttack = 1u << 0,
    kJump = 1u << 1,
    kCrouch = 1u << 2,
    kUse = 1u << 3,
    kReload = 1u << 4,
    kWalk = 1u << 5,
};

[[nodiscard]] constexpr uint32_t buttonBit(EButton button) {
    return static_cast<uint32_t>(button);
}

[[nodiscard]] constexpr bool hasButton(uint32_t mask, EButton button) {
    return (mask & buttonBit(button)) != 0;
}

// ---------------------------------------------------------------------------
// Kullanıcı komutu (bir tick'lik input)
// ---------------------------------------------------------------------------
struct UserCmd_t {
    uint32_t tick = 0;
    float forwardMove = 0.0f;      // -1..1
    float sideMove = 0.0f;         // -1..1
    float upMove = 0.0f;           // -1..1
    float yaw = 0.0f;              // radyan
    float pitch = 0.0f;            // radyan
    uint32_t buttons = 0;          // basılı tutulanlar
    uint32_t buttonsPressed = 0;   // bu tick basılanlar (edge)
    uint32_t buttonsReleased = 0;  // bu tick bırakılanlar (edge)
};

}  // namespace game

namespace game {

// ---------------------------------------------------------------------------
// Kontrol isteği (client -> server): izle, bot'u devral, bırak.
// ---------------------------------------------------------------------------
enum class EControlAction : uint32_t {
    kNone = 0,
    kSpectate = 1,  // targetNetId'li pawn'ı izle (kendi pawn'ını bırakır)
    kTakeOver = 2,  // izlenen bot'un pawn'ını devral
    kRelease = 3,   // izlemeyi / devralmayı bırak, kendi pawn'ına dön
};

struct ControlRequest_t {
    EControlAction action = EControlAction::kNone;
    uint32_t targetNetId = 0;
};

}  // namespace game
