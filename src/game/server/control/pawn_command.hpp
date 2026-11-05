#pragma once

#include "game/shared/world.hpp"

namespace game::control_rules {

// Pawn'ın bu tick uygulayacağı komut: controller'ının komutu. Kimse sürmüyorsa durur (bakışı korur).
[[nodiscard]] UserCmd_t commandOf(const World_t& world, entt::entity pawn);

}  // namespace game::control_rules
