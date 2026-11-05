#include "game/shared/snapshot.hpp"

#include <unordered_set>

namespace game::replication {
namespace {

// Entity'de component varsa optional'a kopyalar.
template <typename Component>
std::optional<Component> copyOf(const entt::registry& registry, entt::entity entity) {
    const auto* component = registry.try_get<Component>(entity);
    return component != nullptr ? std::optional<Component>{*component} : std::nullopt;
}

// Snapshot'ta component varsa entity'ye yazar.
template <typename Component>
void write(entt::registry& registry, entt::entity entity, const std::optional<Component>& value) {
    if (value) {
        registry.emplace_or_replace<Component>(entity, *value);
    }
}

}  // namespace

WorldSnapshot_t capture(const World_t& world) {
    WorldSnapshot_t snapshot{.tick = world.tick, .tickRate = world.tickRate};
    for (const auto [entity, replicated] : world.registry.view<const Replicated_t>().each()) {
        snapshot.entities.push_back({.netId = replicated.netId,
                                     .transform = copyOf<Transform_t>(world.registry, entity),
                                     .velocity = copyOf<Velocity_t>(world.registry, entity),
                                     .player = copyOf<Player_t>(world.registry, entity),
                                     .projectile = copyOf<Projectile_t>(world.registry, entity)});
    }
    return snapshot;
}

void apply(World_t& world, const WorldSnapshot_t& snapshot) {
    world.tick = snapshot.tick;
    if (isValidTickRate(snapshot.tickRate)) {
        world.tickRate = snapshot.tickRate;
    }

    std::unordered_set<uint32_t> alive;
    for (const EntitySnapshot_t& item : snapshot.entities) {
        alive.insert(item.netId);
        const entt::entity entity = world::findOrCreateReplicated(world, item.netId);
        write(world.registry, entity, item.transform);
        write(world.registry, entity, item.velocity);
        write(world.registry, entity, item.player);
        write(world.registry, entity, item.projectile);
    }

    std::vector<entt::entity> gone;
    for (const auto [entity, replicated] : world.registry.view<const Replicated_t>().each()) {
        if (!alive.contains(replicated.netId)) {
            gone.push_back(entity);
        }
    }
    for (const entt::entity entity : gone) {
        world::destroy(world, entity);
    }
}

}  // namespace game::replication
