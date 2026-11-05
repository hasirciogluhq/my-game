#include "game/client/effects/fire_effects.hpp"

namespace game::fire_effects {

void update(const UserCmd_t& cmd, bool freeCam, IParticles& particles, IAudio* audio, const glm::vec3& eye, const glm::vec3& forward) {
    if (freeCam || !hasButton(cmd.buttonsPressed, EButton::kAttack)) {
        return;
    }
    particles.emit(eye + forward * 0.5f, forward * 8.0f, 0.25f);
    if (audio != nullptr) {
        audio->playTone(220.0f, 0.06f, 0.15f);
    }
}

}  // namespace game::fire_effects
