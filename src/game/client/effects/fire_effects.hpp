#pragma once

#include "engine/interface/i_audio.hpp"
#include "engine/interface/i_particles.hpp"
#include "game/shared/game_types.hpp"

#include <glm/vec3.hpp>

// Client-side feedback played when firing (muzzle particle + sound). The bullet itself is on the server
// (game/server/systems/weapon.cpp); this is only the local visual/audio effect.
namespace game::fire_effects {

// Plays the effect if a shot started this tick (kAttack pressed) and free-cam is off. `audio` may be empty.
void update(const UserCmd_t& cmd, bool freeCam, IParticles& particles, IAudio* audio, const glm::vec3& eye, const glm::vec3& forward);

}  // namespace game::fire_effects
