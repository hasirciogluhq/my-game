#include "game/shared/movement_settings.hpp"

namespace game {

MovementSettings_t movementSettingsFrom(const ConfigValues_t& values) {
    const MovementSettings_t defaults;
    return MovementSettings_t{
        .walkSpeed = config::getFloat(values, "move.walk.speed", defaults.walkSpeed),
        .runSpeed = config::getFloat(values, "move.run.speed", defaults.runSpeed),
        .crouchScale = config::getFloat(values, "move.crouch.scale", defaults.crouchScale),
        .jumpSpeed = config::getFloat(values, "move.jump.speed", defaults.jumpSpeed),
        .gravity = config::getFloat(values, "move.gravity", defaults.gravity),
        .maxSpeed = config::getFloat(values, "move.max_speed", defaults.maxSpeed),
        .groundAccelerate = config::getFloat(values, "move.ground.accelerate", defaults.groundAccelerate),
        .groundFriction = config::getFloat(values, "move.ground.friction", defaults.groundFriction),
        .groundStopSpeed = config::getFloat(values, "move.ground.stop_speed", defaults.groundStopSpeed),
        .airAccelerate = config::getFloat(values, "move.air.accelerate", defaults.airAccelerate),
        .airWishCap = config::getFloat(values, "move.air.wish_cap", defaults.airWishCap),
        .bunnyhop = config::getBool(values, "move.bunnyhop.enabled", defaults.bunnyhop),
        .slide = config::getBool(values, "move.slide.enabled", defaults.slide),
        .slideMinSpeed = config::getFloat(values, "move.slide.min_speed", defaults.slideMinSpeed),
        .slideDuration = config::getFloat(values, "move.slide.duration", defaults.slideDuration),
        .slideFriction = config::getFloat(values, "move.slide.friction", defaults.slideFriction),
    };
}

}  // namespace game
