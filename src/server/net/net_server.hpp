#pragma once

#include "engine/interface/i_network.hpp"
#include "engine/interface/network_task.hpp"
#include "game/shared/world.hpp"
#include "server/api.hpp"

namespace game {

// The server's network manager (the counterpart of CNetClient on the client): on top of network.dll's INetwork it holds
// the players' connections (peer id = player id), routes packets to the matching world call, and sends welcome / view target /
// snapshot. Contains no game rules (the rules live in game/server/).
class SERVER_API CNetServer {
public:
    void bind(INetwork* network, World_t* world) {
        m_network = network;
        m_world = world;
    }
    [[nodiscard]] bool listen(uint16_t port);
    void pump();               // delivers network.dll events; the coroutine wakes up and processes them
    void broadcastSnapshot();  // the full state of the world, every tick

private:
    // Coroutine that processes network events in order: wakes up when pump() brings an event.
    net::Task serveNetwork();
    void onPacket(const NetEvent_t& event);
    void sendWelcome(PeerId_t peer);
    void sendViewTarget(PeerId_t peer);

    INetwork* m_network = nullptr;
    World_t* m_world = nullptr;
};

}  // namespace game
