#pragma once

#include "engine/engine/core/types.hpp"
#include "engine/interface/i_physics.hpp"
#include "game/shared/components.hpp"
#include "game/shared/movement_settings.hpp"

#include <cstdint>
#include <unordered_map>

#include <entt/entity/registry.hpp>

namespace game {

// Game world: all entities are in the registry. The server and the client use the same type.
struct World_t {
  entt::registry registry;
  IPhysics *physics = nullptr; // on the client, only for local prediction
  MovementSettings_t movement; // server cvars; the client gets them in WELCOME
  uint32_t tick = 0;
  uint32_t tickRate = kDefaultTickRate;
  uint32_t nextNetId = 1;
  std::unordered_map<uint32_t, entt::entity> byNetId;
};

namespace world {

void reset(World_t &world, uint32_t tickRate);
[[nodiscard]] float tickDelta(const World_t &world);

// Replicated entities (found by netId).
[[nodiscard]] entt::entity createReplicated(World_t &world); // assigns a new netId
[[nodiscard]] entt::entity findOrCreateReplicated(World_t &world,
                                                  uint32_t netId);
void destroy(World_t &world, entt::entity entity);

[[nodiscard]] entt::entity findByNetId(const World_t &world,
                                       uint32_t netId); // may be entt::null

// Players (server). A player = a PlayerController_t entity + the character pawn it drives. spawnPlayer returns the pawn;
// if the player already exists, it returns their own pawn (homePawn).
entt::entity spawnPlayer(World_t &world, uint32_t playerId);
void despawnPlayer(World_t &world, uint32_t playerId);
// Deletes the character pawn (with its physics capsule). The controller stays (when a player dies, the connection and controller remain).
void killPawn(World_t &world, entt::entity pawn);
// Bot: AiController_t + character pawn. turnRate: how fast it turns while patrolling (rad/s). Returns the pawn.
entt::entity spawnBot(World_t &world, const glm::vec3 &position, float turnRate);
// The pawn that the player with playerId is CURRENTLY driving (found from the Player_t replicated to the client).
// May be entt::null.
[[nodiscard]] entt::entity findPlayer(const World_t &world, uint32_t playerId);
// The controller of the player with playerId (exists only on the server). May be
// entt::null.
[[nodiscard]] entt::entity findController(const World_t &world,
                                          uint32_t playerId);
void enqueueCommand(World_t &world, uint32_t playerId, const UserCmd_t &cmd);
// Animated prop (server): replicated; systems::animation picks its state. No controller, no collision.
entt::entity spawnAnimated(World_t &world, EModel model,
                           const glm::vec3 &position, float yaw = 0.0f);

} // namespace world
} // namespace game
