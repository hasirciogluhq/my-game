#include "client/net/net_client.hpp"

#include "engine/interface/i_log.hpp"
#include "game/shared/protocol.hpp"

#include <tuple>
#include <utility>

namespace game {

bool CNetClient::connectTo(const std::string& host, uint16_t port) {
    if (m_network == nullptr || !m_network->connect(host, port)) {
        return false;
    }
    m_playerId.reset();
    m_tickRate.reset();
    m_viewTargetNetId = 0;
    m_snapshots.clear();
    m_lastTick = 0;
    return true;
}

void CNetClient::poll() {
    if (m_network == nullptr) {
        return;
    }
    m_network->pump();
    while (auto event = m_network->nextEvent()) {
        if (event->type == ENetEvent::kPacket) {
            onPacket(*event);
        } else if (event->type == ENetEvent::kDisconnected) {
            GAME_LOG_WARN("client", "server connection lost");
        }
    }
}

void CNetClient::onPacket(const NetEvent_t& event) {
    auto envelope = protocol::decode(event.data);
    if (!envelope) {
        GAME_LOG_WARN("client", "could not decode packet: {}", envelope.error());
        return;
    }
    switch (envelope->type) {
        case netproto::Envelope::TYPE_WELCOME:
            m_playerId = envelope->playerId;
            m_tickRate = envelope->tickRate;
            m_movement = envelope->movement;
            break;
        case netproto::Envelope::TYPE_VIEW_TARGET:
            m_viewTargetNetId = envelope->viewTargetNetId;
            break;
        case netproto::Envelope::TYPE_SNAPSHOT:
            if (envelope->snapshot.tick > m_lastTick) {
                m_lastTick = envelope->snapshot.tick;
                m_snapshots.push_back(std::move(envelope->snapshot));
            }
            break;
        default:
            break;
    }
}

void CNetClient::sendCommand(const UserCmd_t& cmd) {
    if (!connected()) {
        return;
    }
    Envelope_t envelope;
    envelope.type = netproto::Envelope::TYPE_USER_CMD;
    envelope.cmd = cmd;
    if (const auto bytes = protocol::encode(envelope)) {
        // Commands are sent reliably: edge flags (jump/fire) must not be lost.
        std::ignore = m_network->send(kServerPeer, *bytes, EDeliver::kReliable);
    }
}

void CNetClient::sendControl(const ControlRequest_t& request) {
    if (!connected()) {
        return;
    }
    Envelope_t envelope;
    envelope.type = netproto::Envelope::TYPE_CONTROL;
    envelope.control = request;
    if (const auto bytes = protocol::encode(envelope)) {
        std::ignore = m_network->send(kServerPeer, *bytes, EDeliver::kReliable);
    }
}

void CNetClient::disconnect() {
    if (m_network != nullptr) {
        m_network->shutdown();
    }
}

bool CNetClient::connected() const {
    return m_network != nullptr && m_network->connected();
}

std::optional<uint32_t> CNetClient::playerId() const {
    return m_playerId;
}

std::optional<uint32_t> CNetClient::tickRate() const {
    return m_tickRate;
}

std::vector<WorldSnapshot_t> CNetClient::consumeSnapshots() {
    return std::exchange(m_snapshots, {});
}

}  // namespace game
