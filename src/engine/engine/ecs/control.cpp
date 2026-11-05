#include "engine/engine/ecs/control.hpp"

namespace game::control {
namespace {

[[nodiscard]] bool isPawn(const entt::registry& registry, entt::entity entity) {
    return entity != entt::null && registry.valid(entity) && registry.all_of<Pawn_t>(entity);
}

[[nodiscard]] bool isController(const entt::registry& registry, entt::entity entity) {
    return entity != entt::null && registry.valid(entity) && registry.all_of<Controller_t>(entity);
}

}  // namespace

void makePawn(entt::registry& registry, entt::entity entity) {
    registry.emplace_or_replace<Pawn_t>(entity);
}

entt::entity createController(entt::registry& registry, bool protectsPawn) {
    const entt::entity entity = registry.create();
    registry.emplace<Controller_t>(entity, Controller_t{.protectsPawn = protectsPawn});
    return entity;
}

entt::entity pawnOf(const entt::registry& registry, entt::entity controller) {
    if (!isController(registry, controller)) {
        return entt::null;
    }
    const entt::entity pawn = registry.get<Controller_t>(controller).pawn;
    return isPawn(registry, pawn) ? pawn : entt::null;
}

entt::entity controllerOf(const entt::registry& registry, entt::entity pawn) {
    if (!isPawn(registry, pawn)) {
        return entt::null;
    }
    const entt::entity controller = registry.get<Pawn_t>(pawn).controller;
    return isController(registry, controller) ? controller : entt::null;
}

bool possess(entt::registry& registry, entt::entity controller, entt::entity pawn) {
    if (!isController(registry, controller) || !isPawn(registry, pawn)) {
        return false;
    }
    const entt::entity previous = controllerOf(registry, pawn);
    if (previous == controller) {
        return true;
    }
    if (previous != entt::null && registry.get<Controller_t>(previous).protectsPawn) {
        return false;
    }

    unpossess(registry, controller);  // eski pawn'ı bırak (kendi displaced'ine döner)
    clearViewTarget(registry, controller);

    // Eski controller (bot) pawn'ı kaybeder ama geri dönmek üzere hatırlanır.
    if (previous != entt::null) {
        registry.get<Controller_t>(previous).pawn = entt::null;
    }
    registry.get<Pawn_t>(pawn) = Pawn_t{.controller = controller, .displaced = previous};
    registry.get<Controller_t>(controller).pawn = pawn;
    return true;
}

void unpossess(entt::registry& registry, entt::entity controller) {
    const entt::entity pawn = pawnOf(registry, controller);
    if (pawn == entt::null) {
        return;
    }
    registry.get<Controller_t>(controller).pawn = entt::null;
    auto& state = registry.get<Pawn_t>(pawn);
    const entt::entity heir = state.displaced;
    state = Pawn_t{};

    // Pawn eski controller'ına döner (o arada başka pawn almadıysa).
    if (isController(registry, heir) && pawnOf(registry, heir) == entt::null) {
        registry.get<Pawn_t>(pawn).controller = heir;
        registry.get<Controller_t>(heir).pawn = pawn;
    }
}

bool setViewTarget(entt::registry& registry, entt::entity controller, entt::entity target) {
    if (!isController(registry, controller) || !isPawn(registry, target)) {
        return false;
    }
    registry.get<Controller_t>(controller).viewTarget = target;
    return true;
}

void clearViewTarget(entt::registry& registry, entt::entity controller) {
    if (isController(registry, controller)) {
        registry.get<Controller_t>(controller).viewTarget = entt::null;
    }
}

entt::entity viewTarget(const entt::registry& registry, entt::entity controller) {
    if (!isController(registry, controller)) {
        return entt::null;
    }
    const entt::entity watched = registry.get<Controller_t>(controller).viewTarget;
    return isPawn(registry, watched) ? watched : pawnOf(registry, controller);
}

void onDestroy(entt::registry& registry, entt::entity entity) {
    if (isController(registry, entity)) {
        unpossess(registry, entity);
    }
    if (isPawn(registry, entity)) {
        const entt::entity controller = controllerOf(registry, entity);
        if (controller != entt::null) {
            registry.get<Controller_t>(controller).pawn = entt::null;
        }
    }
}

}  // namespace game::control
