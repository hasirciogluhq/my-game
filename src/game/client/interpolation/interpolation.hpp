#pragma once

#include "game/shared/snapshot.hpp"

#include <deque>

// Snapshot interpolation (remote players + projectiles). Game logic: lives in game/, not engine/.
// The client keeps the display a little behind server time (delaySeconds) and blends between two snapshots.
// No extrapolation: the display time never passes the newest snapshot.
namespace game::interpolation {

inline constexpr size_t kMaxSnapshots = 32;
inline constexpr uint32_t kMinDelayTicks = 2;
inline constexpr double kCatchUp = 0.1;       // the display time approaches the target by this fraction per tick
inline constexpr double kJumpThreshold = 3.0; // if it is this many ticks away from the target, jump straight to it

struct Interpolation_t {
    std::deque<WorldSnapshot_t> snapshots;  // in tick order
    double renderTick = 0.0;                // display time in server ticks
    float delaySeconds = 0.05f;             // cl_interp
    bool enabled = true;                    // cl_interpolate
};

// Adds a new snapshot (its tick must be greater than the last one added).
void push(Interpolation_t& state, WorldSnapshot_t snapshot);

// Every client tick: advances the display time and discards old snapshots that are no longer used.
void advance(Interpolation_t& state, uint32_t tickRate);

// The moment shown on screen in server time: renderTick when interpolation is on, world.tick if there is no snapshot yet.
// Things computed from time, like projectile position and animation pose, use this.
[[nodiscard]] double displayTick(const Interpolation_t& state, uint32_t worldTick);

// Writes the Transform_t of every player except the local one, at the blended value at renderTick.
void apply(World_t& world, const Interpolation_t& state, uint32_t localPlayerId);

// Blends angles by the shortest path (wrap-around: pi -> -pi does not turn the wrong way).
[[nodiscard]] float lerpAngle(float from, float to, float t);

}  // namespace game::interpolation
