#pragma once

#include "engine/interface/i_physics.hpp"
#include "engine/physics/physics_system.hpp"

#include <memory>
#include <vector>

namespace game {

// Interface: IPhysicsFactory (PhysicsFactory001). Keeps every world it created alive.
class CPhysicsFactory final : public IPhysicsFactory {
public:
    [[nodiscard]] IPhysics* createWorld() override;

private:
    std::vector<std::unique_ptr<CPhysicsSystem>> m_worlds;
};

}  // namespace game
