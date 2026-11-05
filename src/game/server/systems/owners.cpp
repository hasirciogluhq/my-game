#include "game/server/systems/owners.hpp"

#include "engine/engine/ecs/control.hpp"

namespace game::systems {

void owners(World_t& world) {
    for (auto [pawn, player] : world.registry.view<Player_t>().each()) {
        const entt::entity controller = control::controllerOf(world.registry, pawn);
        const auto* owner = controller != entt::null ? world.registry.try_get<PlayerController_t>(controller) : nullptr;
        player.playerId = owner != nullptr ? owner->playerId : 0;
    }
}

}  // namespace game::systems
