#include "engine/interface/create_interface.hpp"
#include "engine/particles/particle_system.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CParticleSystem, IParticles, kParticlesInterface)

}  // namespace game

GAME_EXPORT_MODULE()
