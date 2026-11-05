#include "game/server/systems/ai.hpp"

namespace game::systems {

void ai(World_t& world) {
    const float dt = world::tickDelta(world);
    for (auto [controller, brain, queue] : world.registry.view<AiController_t, CommandQueue_t>().each()) {
        brain.yaw += brain.turnRate * dt;
        queue.current = UserCmd_t{.forwardMove = 1.0f, .yaw = brain.yaw, .buttons = buttonBit(EButton::kWalk)};
    }
}

}  // namespace game::systems
