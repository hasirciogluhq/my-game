#pragma once

#include <cstddef>
#include <vector>

#include <glm/vec3.hpp>

namespace game {

// particles.dll arayüzü.
class IParticles {
public:
    virtual ~IParticles() = default;
    virtual void emit(const glm::vec3& origin, const glm::vec3& velocity, float lifetime) = 0;
    virtual void update(float dt) = 0;
    virtual void clear() = 0;
    [[nodiscard]] virtual std::size_t count() const = 0;
    // Çizim için canlı parçacıkların konumları.
    [[nodiscard]] virtual std::vector<glm::vec3> livePositions() const = 0;
};

inline constexpr char kParticlesInterface[] = "Particles001";

}  // namespace game
