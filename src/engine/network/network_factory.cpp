#include "engine/network/network_factory.hpp"

namespace game {

INetwork* CNetworkFactory::createNetwork() {
    m_networks.push_back(std::make_unique<CNetworkSystem>());
    return m_networks.back().get();
}

}  // namespace game
