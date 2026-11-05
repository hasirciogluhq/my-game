#pragma once

#include "engine/interface/i_physics.hpp"
#include "engine/physics/api.hpp"

#include <memory>

namespace game {

// Jolt Physics wrapper. Interface: IPhysics (Physics001).
// The world is static (boxes); moving things are CharacterVirtual capsules.
class PHYSICS_API CPhysicsSystem final : public IPhysics {
public:
    CPhysicsSystem();
    ~CPhysicsSystem() override;

    [[nodiscard]] bool initialize() override;
    void shutdown() override;

    void setCharacterCollisions(bool enabled) override;
    void setCharacterPosition(CharacterHandle_t handle, const glm::vec3& feetPosition) override;

    void addStaticBox(const glm::vec3& center, const glm::vec3& halfExtents) override;

    [[nodiscard]] CharacterHandle_t createCharacter(const glm::vec3& feetPosition) override;
    void destroyCharacter(CharacterHandle_t handle) override;
    [[nodiscard]] CharacterResult_t moveCharacter(CharacterHandle_t handle, const glm::vec3& velocity, float dt) override;

    [[nodiscard]] RayHit_t raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const override;

private:
    struct Impl_t;  // keep Jolt types out of the header
    std::unique_ptr<Impl_t> m_impl;
};

}  // namespace game
