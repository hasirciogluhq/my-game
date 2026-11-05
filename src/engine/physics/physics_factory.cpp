#include "engine/physics/physics_factory.hpp"

namespace game {

IPhysics* CPhysicsFactory::createWorld() {
    m_worlds.push_back(std::make_unique<CPhysicsSystem>());
    return m_worlds.back().get();
}

}  // namespace game
