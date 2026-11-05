#include "server/net/net_server.hpp"

#include "engine/interface/i_log.hpp"
#include "game/server/control/control_request.hpp"
#include "game/shared/protocol.hpp"
#include "game/shared/snapshot.hpp"

#include <tuple>

namespace game {

bool CNetServer::listen(uint16_t port) {
    if (!m_network->listen(port)) {
        return false;
    }
    serveNetwork();
    return true;
}

void CNetServer::pump() {
    m_network->pump();
}

net::Task CNetServer::serveNetwork() {
    while (true) {
        const NetEvent_t event = co_await net::nextEvent(*m_network);
        switch (event.type) {
            case ENetEvent::kConnected:
                std::ignore = world::spawnPlayer(*m_world, event.peer);  // oyuncu id'si = peer id
                sendWelcome(event.peer);
                break;
            case ENetEvent::kDisconnected:
                world::despawnPlayer(*m_world, event.peer);
                break;
            case ENetEvent::kPacket:
                onPacket(event);
                break;
        }
    }
}

void CNetServer::onPacket(const NetEvent_t& event) {
    const auto envelope = protocol::decode(event.data);
    if (!envelope) {
        GAME_LOG_WARN("server", "peer {}: paket çözülemedi: {}", event.peer, envelope.error());
        return;
    }
    if (envelope->type == netproto::Envelope::TYPE_USER_CMD) {
        // Paket geldi: sıraya al, bir sonraki server tick'inde işlenir.
        world::enqueueCommand(*m_world, event.peer, envelope->cmd);
    } else if (envelope->type == netproto::Envelope::TYPE_CONTROL) {
        const bool accepted = world::handleControl(*m_world, event.peer, envelope->control);
        GAME_LOG_DEBUG("server", "peer {}: kontrol isteği {} hedef {} -> {}", event.peer, static_cast<uint32_t>(envelope->control.action),
                       envelope->control.targetNetId, accepted ? "kabul" : "red");
        sendViewTarget(event.peer);  // kabul de olsa red de olsa client gerçek durumu öğrenir
    }
}

void CNetServer::sendWelcome(PeerId_t peer) {
    Envelope_t welcome;
    welcome.type = netproto::Envelope::TYPE_WELCOME;
    welcome.playerId = peer;
    welcome.tickRate = m_world->tickRate;
    if (const auto bytes = protocol::encode(welcome)) {
        std::ignore = m_network->send(peer, *bytes, EDeliver::kReliable);
    }
}

void CNetServer::sendViewTarget(PeerId_t peer) {
    Envelope_t message;
    message.type = netproto::Envelope::TYPE_VIEW_TARGET;
    message.viewTargetNetId = world::spectatedNetId(*m_world, peer);
    if (const auto bytes = protocol::encode(message)) {
        std::ignore = m_network->send(peer, *bytes, EDeliver::kReliable);
    }
}

void CNetServer::broadcastSnapshot() {
    Envelope_t envelope;
    envelope.type = netproto::Envelope::TYPE_SNAPSHOT;
    envelope.snapshot = replication::capture(*m_world);
    const auto bytes = protocol::encode(envelope);
    if (!bytes) {
        GAME_LOG_ERROR("server", "snapshot kodlanamadı: {}", bytes.error());
        return;
    }
    // Snapshot her tick tam durumu taşır: kaybı sorun değil, UDP yeter. UDP'ye sığmazsa TCP.
    const EDeliver deliver = bytes->size() <= kMaxUnreliableSize ? EDeliver::kUnreliable : EDeliver::kReliable;
    m_network->broadcast(*bytes, deliver);
}

}  // namespace game
