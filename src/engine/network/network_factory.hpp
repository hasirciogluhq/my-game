#pragma once

#include "engine/interface/i_network.hpp"
#include "engine/network/network_system.hpp"

#include <memory>
#include <vector>

namespace game {

// Interface: INetworkFactory (NetworkFactory001). Keeps every network instance it created alive.
class CNetworkFactory final : public INetworkFactory {
public:
    [[nodiscard]] INetwork* createNetwork() override;

private:
    std::vector<std::unique_ptr<CNetworkSystem>> m_networks;
};

}  // namespace game
