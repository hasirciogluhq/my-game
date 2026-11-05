#include "game/server/systems/death.hpp"

#include "engine/engine/ecs/control.hpp"
#include "engine/interface/i_log.hpp"

#include <vector>

namespace game::systems {
namespace {

void clearWatchers(World_t& world, entt::entity pawn) {
    for (auto [controller, state] : world.registry.view<control::Controller_t>().each()) {
        if (state.viewTarget == pawn) {
            control::clearViewTarget(world.registry, controller);
        }
    }
}

// Bot'un AI controller'ı pawn'ıyla birlikte gider (oyuncu controller'ı gitmez).
void destroyIfAi(World_t& world, entt::entity controller) {
    if (controller != entt::null && world.registry.valid(controller) && world.registry.all_of<AiController_t>(controller)) {
        world::destroy(world, controller);
    }
}

void kill(World_t& world, entt::entity pawn) {
    const uint32_t netId = world.registry.get<Replicated_t>(pawn).netId;
    const entt::entity driver = control::controllerOf(world.registry, pawn);
    const entt::entity displaced = world.registry.get<control::Pawn_t>(pawn).displaced;
    GAME_LOG_INFO("combat", "pawn {} öldü", netId);
    clearWatchers(world, pawn);
    world::killPawn(world, pawn);
    destroyIfAi(world, driver);
    destroyIfAi(world, displaced);
}

}  // namespace

void death(World_t& world) {
    std::vector<entt::entity> dead;
    for (auto [pawn, player] : world.registry.view<const Player_t>().each()) {
        if (player.health == 0) {
            dead.push_back(pawn);
        }
    }
    for (const entt::entity pawn : dead) {
        kill(world, pawn);
    }
}

}  // namespace game::systems
