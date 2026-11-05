#include "game/shared/world.hpp"

#include "engine/engine/ecs/control.hpp"

#include <tuple>

namespace game::world {

void reset(World_t& world, uint32_t tickRate) {
    world.registry.clear();
    world.byNetId.clear();
    world.tick = 0;
    world.nextNetId = 1;
    world.tickRate = isValidTickRate(tickRate) ? tickRate : kDefaultTickRate;
}

float tickDelta(const World_t& world) {
    return tickDeltaFor(world.tickRate);
}

entt::entity findOrCreateReplicated(World_t& world, uint32_t netId) {
    if (const auto it = world.byNetId.find(netId); it != world.byNetId.end()) {
        return it->second;
    }
    const entt::entity entity = world.registry.create();
    world.registry.emplace<Replicated_t>(entity, netId);
    world.byNetId.emplace(netId, entity);
    return entity;
}

entt::entity createReplicated(World_t& world) {
    return findOrCreateReplicated(world, world.nextNetId++);
}

void destroy(World_t& world, entt::entity entity) {
    control::onDestroy(world.registry, entity);  // pawn/controller bağlarını temizle
    if (const auto* replicated = world.registry.try_get<Replicated_t>(entity)) {
        world.byNetId.erase(replicated->netId);
    }
    world.registry.destroy(entity);
}

entt::entity findByNetId(const World_t& world, uint32_t netId) {
    const auto it = world.byNetId.find(netId);
    return it != world.byNetId.end() ? it->second : entt::entity{entt::null};
}

entt::entity findPlayer(const World_t& world, uint32_t playerId) {
    for (const auto [entity, player] : world.registry.view<const Player_t>().each()) {
        if (player.playerId == playerId) {
            return entity;
        }
    }
    return entt::null;
}

entt::entity findController(const World_t& world, uint32_t playerId) {
    for (const auto [entity, controller] : world.registry.view<const PlayerController_t>().each()) {
        if (controller.playerId == playerId) {
            return entity;
        }
    }
    return entt::null;
}

namespace {

// Karakter pawn'ı: replicate edilir, controller'sızdır.
entt::entity createCharacter(World_t& world, const glm::vec3& position) {
    const entt::entity entity = createReplicated(world);
    world.registry.emplace<Transform_t>(entity, position);
    world.registry.emplace<Velocity_t>(entity);
    world.registry.emplace<Player_t>(entity);
    world.registry.emplace<Weapon_t>(entity);
    control::makePawn(world.registry, entity);
    if (world.physics != nullptr) {
        world.registry.emplace<Collider_t>(entity, world.physics->createCharacter(position));
    }
    return entity;
}

void destroyCharacter(World_t& world, entt::entity entity) {
    if (const auto* collider = world.registry.try_get<Collider_t>(entity); collider != nullptr && world.physics != nullptr) {
        world.physics->destroyCharacter(collider->handle);
    }
    destroy(world, entity);
}

}  // namespace

entt::entity spawnPlayer(World_t& world, uint32_t playerId) {
    if (const entt::entity existing = findController(world, playerId); existing != entt::null) {
        return world.registry.get<PlayerController_t>(existing).homePawn;
    }
    const entt::entity pawn = createCharacter(world, {static_cast<float>(playerId) * 1.5f, 0.0f, 0.0f});
    const entt::entity controller = control::createController(world.registry, true);  // oyuncu pawn'ını devrettirmez
    world.registry.emplace<PlayerController_t>(controller, PlayerController_t{.playerId = playerId, .homePawn = pawn});
    world.registry.emplace<CommandQueue_t>(controller);
    std::ignore = control::possess(world.registry, controller, pawn);
    world.registry.get<Player_t>(pawn).playerId = playerId;
    return pawn;
}

void despawnPlayer(World_t& world, uint32_t playerId) {
    const entt::entity controller = findController(world, playerId);
    if (controller == entt::null) {
        return;
    }
    const entt::entity home = world.registry.get<PlayerController_t>(controller).homePawn;
    destroy(world, controller);  // devraldığı bot varsa AI'a geri döner
    if (world.registry.valid(home)) {
        destroyCharacter(world, home);
    }
}

void killPawn(World_t& world, entt::entity pawn) {
    destroyCharacter(world, pawn);
}

entt::entity spawnBot(World_t& world, const glm::vec3& position) {
    const entt::entity pawn = createCharacter(world, position);
    const entt::entity controller = control::createController(world.registry, false);  // oyuncu devralabilir
    world.registry.emplace<AiController_t>(controller);
    world.registry.emplace<CommandQueue_t>(controller);
    std::ignore = control::possess(world.registry, controller, pawn);
    return pawn;
}

entt::entity spawnAnimated(World_t& world, EModel model, const glm::vec3& position, float yaw) {
    const entt::entity entity = createReplicated(world);
    world.registry.emplace<Transform_t>(entity, Transform_t{.position = position, .yaw = yaw});
    world.registry.emplace<Animator_t>(entity, Animator_t{.model = model, .startTick = world.tick, .prevStartTick = world.tick});
    return entity;
}

void enqueueCommand(World_t& world, uint32_t playerId, const UserCmd_t& cmd) {
    const entt::entity controller = findController(world, playerId);
    if (controller != entt::null) {
        world.registry.get<CommandQueue_t>(controller).pending.push_back(cmd);
    }
}

}  // namespace game::world
