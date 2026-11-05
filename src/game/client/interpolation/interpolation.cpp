#include "game/client/interpolation/interpolation.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>

#include <glm/common.hpp>

namespace game::interpolation {
namespace {

const EntitySnapshot_t* findEntity(const WorldSnapshot_t& snapshot, uint32_t netId) {
    const auto it = std::ranges::find(snapshot.entities, netId, &EntitySnapshot_t::netId);
    return it != snapshot.entities.end() ? &*it : nullptr;
}

}  // namespace

float lerpAngle(float from, float to, float t) {
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    float delta = std::fmod(to - from, kTwoPi);
    if (delta > std::numbers::pi_v<float>) {
        delta -= kTwoPi;
    } else if (delta < -std::numbers::pi_v<float>) {
        delta += kTwoPi;
    }
    return from + delta * t;
}

void push(Interpolation_t& state, WorldSnapshot_t snapshot) {
    state.snapshots.push_back(std::move(snapshot));
    if (state.snapshots.size() > kMaxSnapshots) {
        state.snapshots.pop_front();
    }
}

double displayTick(const Interpolation_t& state, uint32_t worldTick) {
    return state.snapshots.empty() ? static_cast<double>(worldTick) : state.renderTick;
}

void advance(Interpolation_t& state, uint32_t tickRate) {
    if (state.snapshots.empty()) {
        return;
    }
    const auto latest = static_cast<double>(state.snapshots.back().tick);
    if (!state.enabled) {
        state.renderTick = latest;  // gecikmesiz: en yeni snapshot aynen
    } else {
        const auto delayTicks = std::max(kMinDelayTicks, static_cast<uint32_t>(std::ceil(state.delaySeconds * static_cast<float>(tickRate))));
        const double target = latest - static_cast<double>(delayTicks);
        const double diff = target - state.renderTick;
        state.renderTick = std::abs(diff) > kJumpThreshold ? target : state.renderTick + 1.0 + kCatchUp * diff;
        state.renderTick = std::min(state.renderTick, latest);  // extrapolation yok
    }

    // Gösterim zamanının solunda kalan, artık kullanılmayacak snapshot'ları at (bir tanesi "a" olarak kalır).
    while (state.snapshots.size() > 2 && static_cast<double>(state.snapshots[1].tick) <= state.renderTick) {
        state.snapshots.pop_front();
    }
}

void apply(World_t& world, const Interpolation_t& state, uint32_t localPlayerId) {
    if (state.snapshots.empty()) {
        return;
    }
    // a.tick <= renderTick <= b.tick olan çift. Aralık dışındaysa uçtaki snapshot'ın değeri kullanılır.
    const auto after = std::ranges::find_if(state.snapshots, [&](const WorldSnapshot_t& s) {
        return static_cast<double>(s.tick) >= state.renderTick;
    });
    const WorldSnapshot_t& b = after != state.snapshots.end() ? *after : state.snapshots.back();
    const WorldSnapshot_t& a = after != state.snapshots.begin() && after != state.snapshots.end() ? *std::prev(after) : b;
    const double span = static_cast<double>(b.tick) - static_cast<double>(a.tick);
    const auto t = span > 0.0 ? static_cast<float>(std::clamp((state.renderTick - static_cast<double>(a.tick)) / span, 0.0, 1.0)) : 1.0f;

    for (const EntitySnapshot_t& item : b.entities) {
        if (!item.transform || !item.player || item.player->playerId == localPlayerId) {
            continue;
        }
        const auto entity = world.byNetId.find(item.netId);
        if (entity == world.byNetId.end()) {
            continue;
        }
        const EntitySnapshot_t* previous = findEntity(a, item.netId);
        if (previous == nullptr || !previous->transform) {
            world.registry.replace<Transform_t>(entity->second, *item.transform);  // yeni doğdu: sadece b
            continue;
        }
        const Transform_t& from = *previous->transform;
        const Transform_t& to = *item.transform;
        world.registry.replace<Transform_t>(entity->second, Transform_t{.position = glm::mix(from.position, to.position, t),
                                                                         .yaw = lerpAngle(from.yaw, to.yaw, t),
                                                                         .pitch = lerpAngle(from.pitch, to.pitch, t)});
    }
}

}  // namespace game::interpolation
