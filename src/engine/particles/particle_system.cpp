#include "engine/particles/particle_system.hpp"

#include <algorithm>

namespace game {
namespace {

constexpr float kParticleGravity = 9.8f;

}  // namespace

void CParticleSystem::emit(const glm::vec3& origin, const glm::vec3& velocity, float lifetime) {
    m_particles.push_back(Particle_t{
        .position = origin,
        .velocity = velocity,
        .lifeLeft = lifetime,
    });
}

void CParticleSystem::update(float dt) {
    for (auto& particle : m_particles) {
        particle.velocity.y -= kParticleGravity * dt;
        particle.position += particle.velocity * dt;
        particle.lifeLeft -= dt;
    }
    std::erase_if(m_particles, [](const Particle_t& particle) { return particle.lifeLeft <= 0.0f; });
}

void CParticleSystem::clear() {
    m_particles.clear();
}

std::size_t CParticleSystem::count() const {
    return m_particles.size();
}

std::vector<glm::vec3> CParticleSystem::livePositions() const {
    std::vector<glm::vec3> positions;
    positions.reserve(m_particles.size());
    for (const auto& particle : m_particles) {
        positions.push_back(particle.position);
    }
    return positions;
}

const std::vector<Particle_t>& CParticleSystem::particles() const {
    return m_particles;
}

}  // namespace game
