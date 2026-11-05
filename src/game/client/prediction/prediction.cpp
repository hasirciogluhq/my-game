#include "game/client/prediction/prediction.hpp"

#include "game/shared/movement.hpp"

#include <cmath>

#include <glm/geometric.hpp>

namespace game::prediction {

void attach(World_t& world, entt::entity self) {
    if (world.physics == nullptr || world.registry.all_of<Collider_t>(self)) {
        return;
    }
    const glm::vec3 feet = world.registry.get<Transform_t>(self).position;
    world.registry.emplace<Collider_t>(self, world.physics->createCharacter(feet));
}

void predict(World_t& world, Prediction_t& state, entt::entity self, const UserCmd_t& cmd) {
    if (!state.enabled) {
        return;
    }
    attach(world, self);
    systems::movePlayer(world, self, cmd);
    state.predicted = world.registry.get<Transform_t>(self).position;
    state.unacked.push_back(cmd);
    if (state.unacked.size() > kMaxUnacked) {
        state.unacked.pop_front();
    }
}

void reconcile(World_t& world, Prediction_t& state, entt::entity self) {
    if (!state.enabled) {
        return;
    }
    attach(world, self);
    const uint32_t acked = world.registry.get<Player_t>(self).lastCmdSeq;
    while (!state.unacked.empty() && state.unacked.front().tick <= acked) {
        state.unacked.pop_front();
    }

    // apply wrote the entity to the server position: move the physics character there too, then replay the pending commands.
    if (const auto* collider = world.registry.try_get<Collider_t>(self); collider != nullptr && state.physics != nullptr) {
        state.physics->setCharacterPosition(collider->handle, world.registry.get<Transform_t>(self).position);
    }
    for (const UserCmd_t& cmd : state.unacked) {
        systems::movePlayer(world, self, cmd);
    }

    const glm::vec3 now = world.registry.get<Transform_t>(self).position;
    if (state.predicted) {
        const glm::vec3 error = *state.predicted - now;
        if (glm::length(error) < kSnapDistance) {
            state.errorOffset += error;  // the view slides smoothly from the old prediction to the new one
        } else {
            state.errorOffset = glm::vec3{0.0f};
        }
    }
    state.predicted = now;
}

void decay(Prediction_t& state, float dt) {
    state.errorOffset *= std::exp(-dt / kSmoothTime);
}

void clear(Prediction_t& state) {
    state.unacked.clear();
    state.predicted.reset();
    state.errorOffset = glm::vec3{0.0f};
}

}  // namespace game::prediction
