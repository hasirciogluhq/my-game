#pragma once

#include "engine/engine/core/config.hpp"

namespace game {

// Movement tuning. Units: metres, seconds.
// Set from the server cvars (assets/config/server.cfg, move.* keys), sent to clients in WELCOME, so prediction matches the server.
struct MovementSettings_t {
    float walkSpeed = 4.0f;          // move.walk.speed        (walk key held)
    float runSpeed = 6.5f;           // move.run.speed         (default: Battlefield-like sprint)
    float crouchScale = 0.5f;        // move.crouch.scale      (speed multiplier while crouching)
    float jumpSpeed = 6.0f;          // move.jump.speed        (vertical take-off speed)
    float gravity = 20.0f;           // move.gravity
    float maxSpeed = 12.0f;          // move.max_speed         (hard ceiling on horizontal speed)
    float groundAccelerate = 10.0f;  // move.ground.accelerate (Source sv_accelerate)
    float groundFriction = 4.0f;     // move.ground.friction   (Source sv_friction)
    float groundStopSpeed = 1.9f;    // move.ground.stop_speed (friction floor, Source sv_stopspeed)
    float airAccelerate = 10.0f;     // move.air.accelerate   (Source sv_airaccelerate)
    float airWishCap = 0.76f;        // move.air.wish_cap     (limits the air gain per tick, Source 30 u/s)
    bool bunnyhop = true;            // move.bunnyhop.enabled (jump on landing keeps speed)
    bool slide = true;               // move.slide.enabled    (crouch while sprinting = slide)
    float slideMinSpeed = 5.0f;      // move.slide.min_speed  (slide only from this speed)
    float slideDuration = 1.2f;      // move.slide.duration   (seconds)
    float slideFriction = 0.5f;      // move.slide.friction   (ground friction multiplier while sliding)
};

// Reads the move.* keys from a server config. Missing keys keep the default above.
[[nodiscard]] MovementSettings_t movementSettingsFrom(const ConfigValues_t& values);

}  // namespace game
