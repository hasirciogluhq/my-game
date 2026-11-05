#pragma once

#include "engine/engine/api.hpp"

#include <entt/entity/registry.hpp>

// Pawn / Controller: a control model with no game rules and no dependence on input.
//  - Pawn: the entity that is CONTROLLED (character, vehicle, camera...).
//  - Controller: the entity that CONTROLS (player, bot...). It drives at most one pawn at a time.
//  - Possess: the controller takes over the pawn. Spectate: the controller watches a pawn (view target) without driving it.
//  - Takeover: taking over a pawn that another controller is driving. The pawn's old controller is remembered as `displaced`
//    so the pawn can be returned to it; on unpossess the pawn goes back to it (when a player releases a bot, the bot keeps walking).
// The contents of Pawn/Controller (input, AI) belong to the game; the engine only keeps the link.
namespace game::control {

struct Pawn_t {
    entt::entity controller = entt::null;
    entt::entity displaced = entt::null;  // controller displaced by a takeover; on unpossess the pawn returns to it
};

struct Controller_t {
    entt::entity pawn = entt::null;
    entt::entity viewTarget = entt::null;  // watched pawn (if empty, the controller's own pawn is watched)
    bool protectsPawn = false;             // true: no other controller can take over this controller's pawn
};

ENGINE_API void makePawn(entt::registry& registry, entt::entity entity);
[[nodiscard]] ENGINE_API entt::entity createController(entt::registry& registry, bool protectsPawn);

// The controller starts driving the pawn. First it releases its old pawn and stops watching.
// If the pawn belongs to another controller it is taken over; if that controller is protected (protectsPawn), it returns false.
ENGINE_API bool possess(entt::registry& registry, entt::entity controller, entt::entity pawn);
// Releases the pawn. If the pawn's `displaced` controller is still free, the pawn returns to it.
ENGINE_API void unpossess(entt::registry& registry, entt::entity controller);

// Returns entt::null for an invalid / unlinked case.
[[nodiscard]] ENGINE_API entt::entity pawnOf(const entt::registry& registry, entt::entity controller);
[[nodiscard]] ENGINE_API entt::entity controllerOf(const entt::registry& registry, entt::entity pawn);

// The controller becomes a spectator of a pawn (does not drive it). Returns false if target is not a pawn.
ENGINE_API bool setViewTarget(entt::registry& registry, entt::entity controller, entt::entity target);
ENGINE_API void clearViewTarget(entt::registry& registry, entt::entity controller);
// The pawn the camera looks at: the watched one (if still valid), otherwise its own pawn.
[[nodiscard]] ENGINE_API entt::entity viewTarget(const entt::registry& registry, entt::entity controller);

// Called BEFORE a pawn or controller is deleted (world::destroy does this): clears the links;
// the pawn of the deleted controller returns to the `displaced` controller.
ENGINE_API void onDestroy(entt::registry& registry, entt::entity entity);

}  // namespace game::control
