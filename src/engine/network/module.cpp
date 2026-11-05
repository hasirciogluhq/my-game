#include "engine/network/network_system.hpp"
#include "engine/interface/create_interface.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CNetworkSystem, INetwork, kNetworkInterface)

}  // namespace game

GAME_EXPORT_MODULE()
