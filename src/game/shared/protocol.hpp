#pragma once

#include "game/shared/game_types.hpp"
#include "game/shared/movement_settings.hpp"
#include "game/shared/snapshot.hpp"

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

#include "game.pb.h"

namespace game {

// A single wire message. The meaning of the fields depends on the type:
//  WELCOME   -> playerId, tickRate, movement
//  USER_CMD  -> cmd
//  SNAPSHOT  -> snapshot
//  CONTROL   -> control (client -> server)
//  VIEW_TARGET -> viewTargetNetId (server -> client)
struct Envelope_t {
    netproto::Envelope::Type type = netproto::Envelope::TYPE_UNSPECIFIED;
    uint32_t playerId = 0;
    uint32_t tickRate = 0;
    MovementSettings_t movement;
    UserCmd_t cmd;
    WorldSnapshot_t snapshot;
    ControlRequest_t control;
    uint32_t viewTargetNetId = 0;
};

// Wire: protobuf serialize -> gzip. Framing is done in network.dll.
namespace protocol {

[[nodiscard]] netproto::Envelope toProto(const Envelope_t& envelope);
[[nodiscard]] Envelope_t fromProto(const netproto::Envelope& proto);
[[nodiscard]] std::expected<std::vector<uint8_t>, std::string> encode(const Envelope_t& envelope);
[[nodiscard]] std::expected<Envelope_t, std::string> decode(std::span<const uint8_t> gzipBytes);

}  // namespace protocol
}  // namespace game
