#pragma once

#include <cstddef>
#include <cstdint>

namespace game {

// ---------------------------------------------------------------------------
// Game constants (units: meters, seconds)
// ---------------------------------------------------------------------------
inline constexpr float kEyeHeight = 1.6f;
inline constexpr float kProjectileSpeed = 60.0f;
inline constexpr float kProjectileLifetime = 3.0f;
inline constexpr float kFireInterval = 0.1f;       // seconds, fire interval
inline constexpr uint32_t kProjectileDamage = 20;  // health lost on a projectile hit
inline constexpr size_t kMaxCmdBacklog = 3;        // max pending commands per player on the server

// ---------------------------------------------------------------------------
// Action buttons. Actions, not keys: it does not matter which key "attack" came from.
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
// User command (one tick's input)
// ---------------------------------------------------------------------------
struct UserCmd_t {
    uint32_t tick = 0;
    float forwardMove = 0.0f;      // -1..1
    float sideMove = 0.0f;         // -1..1
    float upMove = 0.0f;           // -1..1
    float yaw = 0.0f;              // radians
    float pitch = 0.0f;            // radians
    uint32_t buttons = 0;          // held down
    uint32_t buttonsPressed = 0;   // pressed this tick (edge)
    uint32_t buttonsReleased = 0;  // released this tick (edge)
};

}  // namespace game

namespace game {

// ---------------------------------------------------------------------------
// Control request (client -> server): spectate, take over a bot, release.
// ---------------------------------------------------------------------------
enum class EControlAction : uint32_t {
    kNone = 0,
    kSpectate = 1,  // spectate the pawn with targetNetId (releases your own pawn)
    kTakeOver = 2,  // take over the pawn of the bot being watched
    kRelease = 3,   // stop spectating / taking over, return to your own pawn
};

struct ControlRequest_t {
    EControlAction action = EControlAction::kNone;
    uint32_t targetNetId = 0;
};

}  // namespace game
