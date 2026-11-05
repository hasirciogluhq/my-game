#include "game/server/control/control_request.hpp"

#include "engine/engine/ecs/control.hpp"

namespace game::world {

bool handleControl(World_t& world, uint32_t playerId, const ControlRequest_t& request) {
    auto& registry = world.registry;
    const entt::entity controller = findController(world, playerId);
    if (controller == entt::null) {
        return false;
    }
    switch (request.action) {
        case EControlAction::kSpectate: {
            // Watching one's own pawn makes no sense; a missing or non-pawn target is rejected.
            const entt::entity target = findByNetId(world, request.targetNetId);
            if (target == entt::null || target == control::pawnOf(registry, controller)) {
                return false;
            }
            if (!control::setViewTarget(registry, controller, target)) {
                return false;
            }
            control::unpossess(registry, controller);  // while watching, one's own pawn (or a bot) is released
            return true;
        }
        case EControlAction::kTakeOver: {
            // Only the watched pawn can be taken over, and only from a bot (a player's pawn is protected).
            const entt::entity watched = registry.get<control::Controller_t>(controller).viewTarget;
            const entt::entity driver = control::controllerOf(registry, watched);
            if (driver == entt::null || !registry.all_of<AiController_t>(driver)) {
                return false;
            }
            return control::possess(registry, controller, watched);
        }
        case EControlAction::kRelease: {
            control::unpossess(registry, controller);
            control::clearViewTarget(registry, controller);
            const entt::entity home = registry.get<PlayerController_t>(controller).homePawn;
            return registry.valid(home) && control::possess(registry, controller, home);  // a dead player has no pawn
        }
        case EControlAction::kNone:
            return false;
    }
    return false;
}

uint32_t spectatedNetId(const World_t& world, uint32_t playerId) {
    const entt::entity controller = findController(world, playerId);
    if (controller == entt::null) {
        return 0;
    }
    const entt::entity watched = world.registry.get<control::Controller_t>(controller).viewTarget;
    if (watched == entt::null || !world.registry.valid(watched)) {
        return 0;
    }
    const auto* replicated = world.registry.try_get<Replicated_t>(watched);
    return replicated != nullptr ? replicated->netId : 0;
}

}  // namespace game::world
