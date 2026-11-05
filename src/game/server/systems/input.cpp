#include "game/server/systems/input.hpp"

#include "engine/engine/ecs/control.hpp"

namespace game::systems {

void input(World_t& world) {
    for (auto [controller, queue] : world.registry.view<CommandQueue_t>().each()) {
        // Don't let the queue grow (delay would accumulate): the oldest commands over the limit are dropped, and their edge flags
        // (pressed/released) are added to the next command so that a jump or shot is not lost.
        while (queue.pending.size() > kMaxCmdBacklog) {
            const UserCmd_t dropped = queue.pending.front();
            queue.pending.pop_front();
            queue.pending.front().buttonsPressed |= dropped.buttonsPressed;
            queue.pending.front().buttonsReleased |= dropped.buttonsReleased;
        }
        if (!queue.pending.empty()) {
            queue.current = queue.pending.front();
            queue.pending.pop_front();
            // client prediction's ack: the command is written to the pawn it is about to be applied to.
            const entt::entity pawn = control::pawnOf(world.registry, controller);
            if (auto* player = pawn != entt::null ? world.registry.try_get<Player_t>(pawn) : nullptr) {
                player->lastCmdSeq = queue.current.tick;
            }
        } else {
            // If no packet arrived, keep the last command's movement and clear the one-shot edges.
            queue.current.buttonsPressed = 0;
            queue.current.buttonsReleased = 0;
        }
    }
}

}  // namespace game::systems
