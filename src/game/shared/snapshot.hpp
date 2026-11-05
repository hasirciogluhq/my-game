#pragma once

#include "game/shared/world.hpp"

#include <optional>
#include <vector>

namespace game {

// Bir entity'nin ağa giden hali: sahip olduğu replicated component'ler.
struct EntitySnapshot_t {
    uint32_t netId = 0;
    std::optional<Transform_t> transform;
    std::optional<Velocity_t> velocity;
    std::optional<Player_t> player;
    std::optional<Projectile_t> projectile;
};

struct WorldSnapshot_t {
    uint32_t tick = 0;
    uint32_t tickRate = kDefaultTickRate;
    std::vector<EntitySnapshot_t> entities;
};

namespace replication {

[[nodiscard]] WorldSnapshot_t capture(const World_t& world);
// Gelen state aynen uygulanır (rewind/interpolation yok). Snapshot'ta olmayan replicated entity'ler silinir.
void apply(World_t& world, const WorldSnapshot_t& snapshot);

}  // namespace replication
}  // namespace game
