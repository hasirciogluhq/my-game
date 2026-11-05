#pragma once

#include "game/shared/game_types.hpp"

#include <cstdint>
#include <deque>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

namespace game {

// Components hold only data (ECS). The logic lives in the system functions in game/shared and game/server.

struct Transform_t {
    glm::vec3 position{0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct Velocity_t {
    glm::vec3 linear{0.0f};
};

// The character pawn currently driven by a player (or a bot). The Pawn/Controller link is in the engine (engine/ecs/control.hpp).
struct Player_t {
    // id of the player currently driving this pawn; 0 if nobody (or a bot) drives it. Synced from the controller every tick.
    uint32_t playerId = 0;
    uint32_t health = 100;
    bool onGround = true;
    // Sequence number of the last command the server processed (UserCmd_t::tick). Client prediction acks with this.
    uint32_t lastCmdSeq = 0;
    // Seconds of slide left (0 = not sliding). Predicted and replicated like the velocity.
    float slideTime = 0.0f;
};

// The projectile position is not updated every tick: pos(t) = origin + velocity * (t - spawnTick) * dt.
struct Projectile_t {
    uint32_t ownerId = 0;  // netId of the pawn that fired it
    uint32_t spawnTick = 0;
    glm::vec3 origin{0.0f};
    glm::vec3 velocity{0.0f};
};

// Animation state chosen by the server. The client does not know clip names or durations; it maps the state to a clip in its own model.
// kGreet: greeting animation for a nearby player; the model's definition decides which clip it is
// (robot: wave, dancer: breakdance). The server only chooses the state.
enum class EAnimState : uint32_t {
    kIdle,
    kGreet,
};

enum class EModel : uint32_t {
    kRobot,
    kDancer,
};

// Animated (skinned) entity. The server does not update the pose every tick: it writes only when the state changes.
// The client computes the pose: clip time = (display tick - startTick) * dt. When the state changes, it blends from the previous state
// over kAnimBlendSeconds (client/scene/scene.hpp) (prevState keeps playing from prevStartTick).
struct Animator_t {
    EModel model = EModel::kRobot;
    EAnimState state = EAnimState::kIdle;
    uint32_t startTick = 0;
    EAnimState prevState = EAnimState::kIdle;
    uint32_t prevStartTick = 0;
};

// Entities carrying this are copied over the network. netId is assigned on the server and used as-is on the client.
struct Replicated_t {
    uint32_t netId = 0;
};

// Character capsule in the physics engine (only while physics is on, i.e. on the server).
struct Collider_t {
    uint32_t handle = 0;
};

// The pawn's weapon on the server (not replicated). The cooldown stays on the pawn; it is not reset when the controller changes.
struct Weapon_t {
    uint32_t nextFireTick = 0;
};

// Player controller (the controller entity; not replicated). homePawn: the player's own character;
// when a bot is released (release) the player goes back here.
struct PlayerController_t {
    uint32_t playerId = 0;
    entt::entity homePawn = entt::null;
};

// Bot controller: simple patrol (walk forward, turn slowly).
struct AiController_t {
    float turnRate = 0.6f;  // rad/s
    float yaw = 0.0f;
};

// Commands arriving at the controller (player: from the network, bot: from AI). current: the command applied to the pawn this tick.
struct CommandQueue_t {
    std::deque<UserCmd_t> pending;
    UserCmd_t current;
};

}  // namespace game
