#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace game {

// physics.dll interface (Jolt). Static world + character capsules + ray test.
// CharacterHandle_t 0 is invalid.
using CharacterHandle_t = uint32_t;

// Character capsule: origin at the feet, total height from the feet to the top. Physics and the ray test use the same values.
inline constexpr float kCharacterRadius = 0.3f;
inline constexpr float kCharacterHeight = 1.8f;

struct RayHit_t {
    bool hit = false;
    float distance = 0.0f;
    glm::vec3 point{0.0f};
};

struct CharacterResult_t {
    glm::vec3 position{0.0f};  // feet position
    bool onGround = false;
};

class IPhysics {
public:
    virtual ~IPhysics() = default;
    [[nodiscard]] virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void addStaticBox(const glm::vec3& center, const glm::vec3& halfExtents) = 0;

    [[nodiscard]] virtual CharacterHandle_t createCharacter(const glm::vec3& feetPosition) = 0;
    virtual void destroyCharacter(CharacterHandle_t handle) = 0;
    // Moves the character by velocity * dt; it collides with walls/ground/boxes and slides along them.
    [[nodiscard]] virtual CharacterResult_t moveCharacter(CharacterHandle_t handle, const glm::vec3& velocity, float dt) = 0;

    [[nodiscard]] virtual RayHit_t raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const = 0;

    // When enabled, characters cannot pass through each other (server setting player.collisions.enabled). Default: off.
    virtual void setCharacterCollisions(bool enabled) = 0;
    // Teleports the character to the feet position (velocity is reset). Used by client prediction's reconcile step.
    virtual void setCharacterPosition(CharacterHandle_t handle, const glm::vec3& feetPosition) = 0;
};

inline constexpr char kPhysicsInterface[] = "Physics001";

}  // namespace game
