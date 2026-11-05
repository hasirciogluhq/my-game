#include "game/server/systems/movement.hpp"

#include "game/server/control/pawn_command.hpp"
#include "game/shared/movement.hpp"

namespace game::systems {

void movement(World_t& world) {
    for (const auto [pawn, player] : world.registry.view<const Player_t>().each()) {
        movePlayer(world, pawn, control_rules::commandOf(world, pawn));
    }
}

}  // namespace game::systems
