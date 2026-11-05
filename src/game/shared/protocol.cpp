#include "game/shared/protocol.hpp"

#include "engine/engine/net/gzip.hpp"

#include <span>

namespace game::protocol {
namespace {

void writeVec3(netproto::Vec3* proto, const glm::vec3& value) {
    proto->set_x(value.x);
    proto->set_y(value.y);
    proto->set_z(value.z);
}

glm::vec3 readVec3(const netproto::Vec3& proto) {
    return {proto.x(), proto.y(), proto.z()};
}

netproto::UserCmd toProtoCmd(const UserCmd_t& cmd) {
    netproto::UserCmd proto;
    proto.set_tick(cmd.tick);
    proto.set_forward_move(cmd.forwardMove);
    proto.set_side_move(cmd.sideMove);
    proto.set_up_move(cmd.upMove);
    proto.set_yaw(cmd.yaw);
    proto.set_pitch(cmd.pitch);
    proto.set_buttons(cmd.buttons);
    proto.set_buttons_pressed(cmd.buttonsPressed);
    proto.set_buttons_released(cmd.buttonsReleased);
    return proto;
}

UserCmd_t fromProtoCmd(const netproto::UserCmd& proto) {
    return UserCmd_t{
        .tick = proto.tick(),
        .forwardMove = proto.forward_move(),
        .sideMove = proto.side_move(),
        .upMove = proto.up_move(),
        .yaw = proto.yaw(),
        .pitch = proto.pitch(),
        .buttons = proto.buttons(),
        .buttonsPressed = proto.buttons_pressed(),
        .buttonsReleased = proto.buttons_released(),
    };
}


netproto::MovementSettings toProtoMovement(const MovementSettings_t& settings) {
    netproto::MovementSettings proto;
    proto.set_walk_speed(settings.walkSpeed);
    proto.set_run_speed(settings.runSpeed);
    proto.set_crouch_scale(settings.crouchScale);
    proto.set_jump_speed(settings.jumpSpeed);
    proto.set_gravity(settings.gravity);
    proto.set_max_speed(settings.maxSpeed);
    proto.set_ground_accelerate(settings.groundAccelerate);
    proto.set_ground_friction(settings.groundFriction);
    proto.set_ground_stop_speed(settings.groundStopSpeed);
    proto.set_air_accelerate(settings.airAccelerate);
    proto.set_air_wish_cap(settings.airWishCap);
    proto.set_bunnyhop(settings.bunnyhop);
    proto.set_slide(settings.slide);
    proto.set_slide_min_speed(settings.slideMinSpeed);
    proto.set_slide_duration(settings.slideDuration);
    proto.set_slide_friction(settings.slideFriction);
    return proto;
}

MovementSettings_t fromProtoMovement(const netproto::MovementSettings& proto) {
    return MovementSettings_t{
        .walkSpeed = proto.walk_speed(),
        .runSpeed = proto.run_speed(),
        .crouchScale = proto.crouch_scale(),
        .jumpSpeed = proto.jump_speed(),
        .gravity = proto.gravity(),
        .maxSpeed = proto.max_speed(),
        .groundAccelerate = proto.ground_accelerate(),
        .groundFriction = proto.ground_friction(),
        .groundStopSpeed = proto.ground_stop_speed(),
        .airAccelerate = proto.air_accelerate(),
        .airWishCap = proto.air_wish_cap(),
        .bunnyhop = proto.bunnyhop(),
        .slide = proto.slide(),
        .slideMinSpeed = proto.slide_min_speed(),
        .slideDuration = proto.slide_duration(),
        .slideFriction = proto.slide_friction(),
    };
}

void writeEntity(netproto::Entity* proto, const EntitySnapshot_t& entity) {
    proto->set_net_id(entity.netId);
    if (entity.transform) {
        auto* item = proto->mutable_transform();
        writeVec3(item->mutable_position(), entity.transform->position);
        item->set_yaw(entity.transform->yaw);
        item->set_pitch(entity.transform->pitch);
    }
    if (entity.velocity) {
        writeVec3(proto->mutable_velocity()->mutable_linear(), entity.velocity->linear);
    }
    if (entity.player) {
        auto* item = proto->mutable_player();
        item->set_player_id(entity.player->playerId);
        item->set_health(entity.player->health);
        item->set_on_ground(entity.player->onGround);
        item->set_last_cmd_seq(entity.player->lastCmdSeq);
        item->set_slide_time(entity.player->slideTime);
    }
    if (entity.projectile) {
        auto* item = proto->mutable_projectile();
        item->set_owner_id(entity.projectile->ownerId);
        item->set_spawn_tick(entity.projectile->spawnTick);
        writeVec3(item->mutable_origin(), entity.projectile->origin);
        writeVec3(item->mutable_velocity(), entity.projectile->velocity);
    }
    if (entity.animator) {
        auto* item = proto->mutable_animator();
        item->set_model(static_cast<uint32_t>(entity.animator->model));
        item->set_state(static_cast<uint32_t>(entity.animator->state));
        item->set_start_tick(entity.animator->startTick);
        item->set_prev_state(static_cast<uint32_t>(entity.animator->prevState));
        item->set_prev_start_tick(entity.animator->prevStartTick);
    }
}

EntitySnapshot_t readEntity(const netproto::Entity& proto) {
    EntitySnapshot_t entity{.netId = proto.net_id()};
    if (proto.has_transform()) {
        entity.transform = Transform_t{.position = readVec3(proto.transform().position()),
                                       .yaw = proto.transform().yaw(),
                                       .pitch = proto.transform().pitch()};
    }
    if (proto.has_velocity()) {
        entity.velocity = Velocity_t{.linear = readVec3(proto.velocity().linear())};
    }
    if (proto.has_player()) {
        entity.player = Player_t{.playerId = proto.player().player_id(),
                                 .health = proto.player().health(),
                                 .onGround = proto.player().on_ground(),
                                 .lastCmdSeq = proto.player().last_cmd_seq(),
                                 .slideTime = proto.player().slide_time()};
    }
    if (proto.has_projectile()) {
        entity.projectile = Projectile_t{.ownerId = proto.projectile().owner_id(),
                                         .spawnTick = proto.projectile().spawn_tick(),
                                         .origin = readVec3(proto.projectile().origin()),
                                         .velocity = readVec3(proto.projectile().velocity())};
    }
    if (proto.has_animator()) {
        entity.animator = Animator_t{.model = static_cast<EModel>(proto.animator().model()),
                                     .state = static_cast<EAnimState>(proto.animator().state()),
                                     .startTick = proto.animator().start_tick(),
                                     .prevState = static_cast<EAnimState>(proto.animator().prev_state()),
                                     .prevStartTick = proto.animator().prev_start_tick()};
    }
    return entity;
}

}  // namespace

netproto::Envelope toProto(const Envelope_t& envelope) {
    netproto::Envelope proto;
    proto.set_type(envelope.type);
    proto.set_player_id(envelope.playerId);
    proto.set_tick_rate(envelope.tickRate);
    *proto.mutable_cmd() = toProtoCmd(envelope.cmd);
    proto.mutable_control()->set_action(static_cast<uint32_t>(envelope.control.action));
    proto.mutable_control()->set_target_net_id(envelope.control.targetNetId);
    proto.set_view_target_net_id(envelope.viewTargetNetId);
    *proto.mutable_movement() = toProtoMovement(envelope.movement);

    auto* snapshot = proto.mutable_snapshot();
    snapshot->set_tick(envelope.snapshot.tick);
    snapshot->set_tick_rate(envelope.snapshot.tickRate);
    for (const auto& entity : envelope.snapshot.entities) {
        writeEntity(snapshot->add_entities(), entity);
    }
    return proto;
}

Envelope_t fromProto(const netproto::Envelope& proto) {
    Envelope_t envelope;
    envelope.type = proto.type();
    envelope.playerId = proto.player_id();
    envelope.tickRate = proto.tick_rate();
    envelope.cmd = fromProtoCmd(proto.cmd());
    envelope.control = {.action = static_cast<EControlAction>(proto.control().action()), .targetNetId = proto.control().target_net_id()};
    envelope.viewTargetNetId = proto.view_target_net_id();
    envelope.movement = fromProtoMovement(proto.movement());
    envelope.snapshot.tick = proto.snapshot().tick();
    envelope.snapshot.tickRate = proto.snapshot().tick_rate();
    for (const auto& item : proto.snapshot().entities()) {
        envelope.snapshot.entities.push_back(readEntity(item));
    }
    return envelope;
}

std::expected<std::vector<uint8_t>, std::string> encode(const Envelope_t& envelope) {
    const auto proto = toProto(envelope);
    std::string raw;
    if (!proto.SerializeToString(&raw)) {
        return std::unexpected("protobuf serialize failed");
    }
    return gzip::compress(std::span<const uint8_t>{reinterpret_cast<const uint8_t*>(raw.data()), raw.size()});
}

std::expected<Envelope_t, std::string> decode(std::span<const uint8_t> gzipBytes) {
    auto raw = gzip::decompress(gzipBytes);
    if (!raw) {
        return std::unexpected(raw.error());
    }

    netproto::Envelope proto;
    if (!proto.ParseFromArray(raw->data(), static_cast<int>(raw->size()))) {
        return std::unexpected("protobuf parse failed");
    }
    return fromProto(proto);
}

}  // namespace game::protocol
