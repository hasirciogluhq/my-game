#include "engine/interface/create_interface.hpp"
#include "engine/platform/platform.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CPlatform, IPlatform, kPlatformInterface)

}  // namespace game

GAME_EXPORT_MODULE()
