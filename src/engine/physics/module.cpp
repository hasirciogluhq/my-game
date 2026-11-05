#include "engine/interface/create_interface.hpp"
#include "engine/physics/physics_factory.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CPhysicsFactory, IPhysicsFactory, kPhysicsFactoryInterface)

}  // namespace game

GAME_EXPORT_MODULE()
