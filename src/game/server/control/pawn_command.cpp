#include "game/server/control/pawn_command.hpp"

#include "engine/engine/ecs/control.hpp"

namespace game::control_rules {

UserCmd_t commandOf(const World_t& world, entt::entity pawn) {
    const entt::entity controller = control::controllerOf(world.registry, pawn);
    if (controller != entt::null) {
        if (const auto* queue = world.registry.try_get<CommandQueue_t>(controller)) {
            return queue->current;
        }
    }
    const auto& transform = world.registry.get<Transform_t>(pawn);
    return UserCmd_t{.yaw = transform.yaw, .pitch = transform.pitch};
}

}  // namespace game::control_rules
