#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

inline constexpr float kGreetEnterRadius = 5.0f;  // waves at a player who enters this distance
inline constexpr float kGreetExitRadius = 6.5f;   // stops once the player moves beyond this distance (prevents flicker)
inline constexpr float kLookRadius = 12.0f;       // turns toward the nearest player from this distance on
inline constexpr float kTurnSpeed = 3.0f;         // rad/s

// Behavior of animated entities (server): turns toward the nearest live player and greets (kGreet) when close.
// On a state change it writes the new state + startTick into Animator_t; the client computes the pose from these two.
void animation(World_t& world);

}  // namespace game::systems
