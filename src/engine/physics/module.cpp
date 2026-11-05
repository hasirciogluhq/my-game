#include "engine/interface/create_interface.hpp"
#include "engine/physics/physics_system.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CPhysicsSystem, IPhysics, kPhysicsInterface)

}  // namespace game

GAME_EXPORT_MODULE()
