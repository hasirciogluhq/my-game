#pragma once

#include "game/shared/world.hpp"

#include <optional>
#include <vector>

namespace game {

// What one entity looks like on its way to the network: the replicated components it owns.
struct EntitySnapshot_t {
    uint32_t netId = 0;
    std::optional<Transform_t> transform;
    std::optional<Velocity_t> velocity;
    std::optional<Player_t> player;
    std::optional<Projectile_t> projectile;
    std::optional<Animator_t> animator;
};

struct WorldSnapshot_t {
    uint32_t tick = 0;
    uint32_t tickRate = kDefaultTickRate;
    std::vector<EntitySnapshot_t> entities;
};

namespace replication {

[[nodiscard]] WorldSnapshot_t capture(const World_t& world);
// The incoming state is applied as-is (no rewind/interpolation). Replicated entities missing from the snapshot are deleted.
void apply(World_t& world, const WorldSnapshot_t& snapshot);

}  // namespace replication
}  // namespace game
