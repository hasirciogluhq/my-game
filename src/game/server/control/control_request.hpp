#pragma once

#include "game/shared/world.hpp"

namespace game::world {

// The client's control request (spectate / take over a bot / release). If it breaks the rules, returns false and nothing changes.
bool handleControl(World_t& world, uint32_t playerId, const ControlRequest_t& request);
// netId of the pawn the player is watching; 0 if not watching.
[[nodiscard]] uint32_t spectatedNetId(const World_t& world, uint32_t playerId);

}  // namespace game::world
