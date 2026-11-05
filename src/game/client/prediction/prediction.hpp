#pragma once

#include "game/shared/world.hpp"

#include <deque>
#include <optional>

#include <glm/vec3.hpp>

// Client-side prediction (only the local player's movement). Game logic: lives in game/, not engine/.
// Flow: every client tick `predict` (the same movePlayer code) -> when a snapshot arrives, replication::apply
// overwrites the local entity with the server value -> right after, `reconcile` replays the commands not yet acked.
namespace game::prediction {

inline constexpr size_t kMaxUnacked = 256;
inline constexpr float kSnapDistance = 2.0f;   // a deviation larger than this is corrected instantly (teleport)
inline constexpr float kSmoothTime = 0.1f;     // errorOffset decay time constant (seconds)

struct Prediction_t {
    std::deque<UserCmd_t> unacked;       // commands the server has not acked yet
    std::optional<glm::vec3> predicted;  // last predicted feet position (empty if there is no prediction yet)
    glm::vec3 errorOffset{0.0f};         // visual correction: added to the camera/draw position, decays over time
    bool enabled = true;                 // cl_predict
    IPhysics* physics = nullptr;      // the client's own physics instance (for teleporting); the same object as world.physics
};

// Gives the local player a physics character (if the client has physics). Can be called several times.
void attach(World_t& world, entt::entity self);

// Applies this tick's command to the local player and adds it to unacked.
void predict(World_t& world, Prediction_t& state, entt::entity self, const UserCmd_t& cmd);

// Called RIGHT AFTER replication::apply: drops the acked ones and replays the rest from the server position.
void reconcile(World_t& world, Prediction_t& state, entt::entity self);

// Decays errorOffset.
void decay(Prediction_t& state, float dt);

// Forgets the prediction history (when cl_predict changes / on reconnect).
void clear(Prediction_t& state);

}  // namespace game::prediction
