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

// Client'ın network yöneticisi: network.dll'in (INetwork) üstünde server ile konuşur (welcome, komut, snapshot).
// Bağlantının kendisi (soketler, TCP/UDP) network.dll'dedir.
class CLIENT_API CNetClient {
public:
    void bind(INetwork* network) { m_network = network; }
    [[nodiscard]] bool connectTo(const std::string& host, uint16_t port);
    void poll();
    void sendCommand(const UserCmd_t& cmd);
    // Izle / bot'u devral / bırak. Server cevabı viewTargetNetId() ile gelir.
    void sendControl(const ControlRequest_t& request);
    void disconnect();

    [[nodiscard]] bool connected() const;
    [[nodiscard]] std::optional<uint32_t> playerId() const;
    // Server'ın söylediği izlenen pawn'ın netId'si; 0 = kimseyi izlemiyor (kendi pawn'ımızdayız).
    [[nodiscard]] uint32_t viewTargetNetId() const { return m_viewTargetNetId; }
    // Server'ın otoriter tick rate'i (welcome paketinden).
    [[nodiscard]] std::optional<uint32_t> tickRate() const;
    // Son çağrıdan beri gelen tüm snapshot'lar, tick sırasıyla (interpolation hepsini kullanır).
    [[nodiscard]] std::vector<WorldSnapshot_t> consumeSnapshots();

private:
    void onPacket(const NetEvent_t& event);

    INetwork* m_network = nullptr;
    std::optional<uint32_t> m_playerId;
    std::optional<uint32_t> m_tickRate;
    uint32_t m_viewTargetNetId = 0;
    std::vector<WorldSnapshot_t> m_snapshots;
    uint32_t m_lastTick = 0;  // bundan eski/aynı tick'li snapshot atılır (UDP sırasız gelebilir)
};

}  // namespace game
