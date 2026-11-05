#pragma once

#include "game/shared/world.hpp"

namespace game::control_rules {

// The command the pawn applies this tick: its controller's command. If nobody drives it, it stops (keeping its view).
[[nodiscard]] UserCmd_t commandOf(const World_t& world, entt::entity pawn);

}  // namespace game::control_rules
