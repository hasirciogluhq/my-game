#pragma once

#include "client/api.hpp"
#include "engine/interface/i_network.hpp"
#include "game/shared/game_types.hpp"
#include "game/shared/snapshot.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace game {

// The client's network manager: talks to the server on top of network.dll's INetwork (welcome, command, snapshot).
// The connection itself (sockets, TCP/UDP) lives in network.dll.
class CLIENT_API CNetClient {
public:
    void bind(INetwork* network) { m_network = network; }
    [[nodiscard]] bool connectTo(const std::string& host, uint16_t port);
    void poll();
    void sendCommand(const UserCmd_t& cmd);
    // Spectate / take over a bot / release. The server's reply arrives through viewTargetNetId().
    void sendControl(const ControlRequest_t& request);
    void disconnect();

    [[nodiscard]] bool connected() const;
    [[nodiscard]] std::optional<uint32_t> playerId() const;
    // netId of the watched pawn as the server reports it; 0 = not watching anyone (we are in our own pawn).
    [[nodiscard]] uint32_t viewTargetNetId() const { return m_viewTargetNetId; }
    // The server's authoritative tick rate (from the welcome packet).
    [[nodiscard]] std::optional<uint32_t> tickRate() const;
    // The server's movement cvars (from the welcome packet): the client predicts with these.
    [[nodiscard]] std::optional<MovementSettings_t> movement() const { return m_movement; }
    // The map the server runs (from the welcome packet).
    [[nodiscard]] std::optional<EMap> map() const { return m_map; }
    // All snapshots received since the last call, in tick order (interpolation uses all of them).
    [[nodiscard]] std::vector<WorldSnapshot_t> consumeSnapshots();

private:
    void onPacket(const NetEvent_t& event);

    INetwork* m_network = nullptr;
    std::optional<uint32_t> m_playerId;
    std::optional<uint32_t> m_tickRate;
    std::optional<MovementSettings_t> m_movement;
    std::optional<EMap> m_map;
    uint32_t m_viewTargetNetId = 0;
    std::vector<WorldSnapshot_t> m_snapshots;
    uint32_t m_lastTick = 0;  // snapshots with this tick or an older one are dropped (UDP can arrive out of order)
};

}  // namespace game
