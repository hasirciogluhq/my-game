#include "engine/network/network_factory.hpp"
#include "engine/interface/create_interface.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CNetworkFactory, INetworkFactory, kNetworkFactoryInterface)

}  // namespace game

GAME_EXPORT_MODULE()
