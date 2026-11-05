#pragma once

#include "engine/interface/i_particles.hpp"
#include "engine/particles/api.hpp"

#include <vector>

#include <glm/vec3.hpp>

namespace game {

struct Particle_t {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float lifeLeft = 0.0f;
};

// CPU-side particle simulation. Interface: IParticles (Particles001).
class PARTICLES_API CParticleSystem final : public IParticles {
public:
    void emit(const glm::vec3& origin, const glm::vec3& velocity, float lifetime) override;
    void update(float dt) override;
    void clear() override;
    [[nodiscard]] std::size_t count() const override;
    [[nodiscard]] std::vector<glm::vec3> livePositions() const override;

    [[nodiscard]] const std::vector<Particle_t>& particles() const;

private:
    std::vector<Particle_t> m_particles;
};

}  // namespace game
