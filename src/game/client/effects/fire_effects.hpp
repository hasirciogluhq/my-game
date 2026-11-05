#pragma once

#include "engine/interface/i_audio.hpp"
#include "engine/interface/i_particles.hpp"
#include "game/shared/game_types.hpp"

#include <glm/vec3.hpp>

// Ateş edince client'ta çalan geri bildirim (namlu parçacığı + ses). Mermi kendisi server'dadır
// (game/server/systems/weapon.cpp); burası sadece yerel görsel/işitsel efekt.
namespace game::fire_effects {

// Bu tick ateş başladıysa (kAttack pressed) ve free-cam kapalıysa efekti çalar. `audio` boş olabilir.
void update(const UserCmd_t& cmd, bool freeCam, IParticles& particles, IAudio* audio, const glm::vec3& eye, const glm::vec3& forward);

}  // namespace game::fire_effects
