// Headless integration test: physics.dll + ECS systems + replication + protobuf.
// Run: xmake run ecs_test
#include "engine/engine/core/ray.hpp"
#include "engine/interface/create_interface.hpp"
#include "engine/interface/module_loader.hpp"
#include "game/client/interpolation/interpolation.hpp"
#include "game/client/prediction/prediction.hpp"
#include "engine/engine/ecs/control.hpp"
#include "game/server/control/control_request.hpp"
#include "game/server/world_tick.hpp"
#include "game/shared/level.hpp"
#include "game/shared/movement.hpp"
#include "game/shared/protocol.hpp"
#include "game/shared/snapshot.hpp"
#include "game/shared/trace.hpp"
#include "game/shared/world.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <numbers>

namespace {

int g_failures = 0;

void check(bool ok, const char* what) {
    std::cout << (ok ? "ok    " : "FAIL  ") << what << "\n";
    g_failures += ok ? 0 : 1;
}

game::UserCmd_t forwardCmd() {
    game::UserCmd_t cmd;
    cmd.forwardMove = 1.0f;  // yaw=0: walk toward -Z
    return cmd;
}

// Movement rules without physics (flat ground at y=0): acceleration, friction, slide, bunnyhop.
void movementChecks() {
    game::World_t world;
    game::world::reset(world, 64);
    const auto pawn = game::world::spawnPlayer(world, 1);
    auto& velocity = world.registry.get<game::Velocity_t>(pawn).linear;
    auto& player = world.registry.get<game::Player_t>(pawn);
    player.onGround = true;

    // Accelerates toward run speed, does not overshoot it.
    for (int i = 0; i < 64; ++i) {
        game::systems::movePlayer(world, pawn, forwardCmd());
    }
    const float runSpeed = world.movement.runSpeed;
    check(std::abs(std::hypot(velocity.x, velocity.z) - runSpeed) < 0.05f, "movement: reaches run speed (move.run.speed)");

    // Lets go: friction brings it to a stop.
    for (int i = 0; i < 128; ++i) {
        game::systems::movePlayer(world, pawn, game::UserCmd_t{});
    }
    check(std::hypot(velocity.x, velocity.z) < 0.01f, "movement: friction stops the player");

    // Sprint, then crouch pressed: starts a slide that lasts move.slide.duration.
    for (int i = 0; i < 64; ++i) {
        game::systems::movePlayer(world, pawn, forwardCmd());
    }
    game::UserCmd_t crouch = forwardCmd();
    crouch.buttons = game::buttonBit(game::EButton::kCrouch);
    crouch.buttonsPressed = crouch.buttons;
    game::systems::movePlayer(world, pawn, crouch);
    check(player.slideTime > 0.0f, "slide: crouch at speed starts a slide");
    for (int i = 0; i < 200; ++i) {
        game::systems::movePlayer(world, pawn, forwardCmd());
    }
    check(player.slideTime == 0.0f, "slide: ends after move.slide.duration");

    // Bunnyhop: jumping on landing keeps the speed (no friction on the jump tick).
    game::UserCmd_t jump = forwardCmd();
    jump.buttons = game::buttonBit(game::EButton::kJump);
    jump.buttonsPressed = jump.buttons;
    for (int i = 0; i < 64; ++i) {
        game::systems::movePlayer(world, pawn, forwardCmd());
    }
    const float beforeJump = std::hypot(velocity.x, velocity.z);
    game::systems::movePlayer(world, pawn, jump);
    check(std::abs(std::hypot(velocity.x, velocity.z) - beforeJump) < 0.05f, "bunnyhop: jump keeps the horizontal speed");
    check(velocity.y > 0.0f, "jump: vertical speed set");
    world.movement.bunnyhop = false;
    player.onGround = true;
    game::systems::movePlayer(world, pawn, jump);
    check(std::hypot(velocity.x, velocity.z) <= runSpeed + 0.01f, "bunnyhop off: jump caps the speed at run speed");
}

}  // namespace

int main(int argc, char** argv) {
    (void)argc;
    movementChecks();
    // Engine modules are under out/.../engine; the test exe is under out/.../game.
    const std::filesystem::path engineDir = std::filesystem::absolute(argv[0]).parent_path().parent_path() / "engine";
    game::CModule physicsModule;
    if (!physicsModule.load(engineDir / game::moduleFileName("physics"))) {
        return 1;
    }
    auto* physicsFactory = physicsModule.getInterface<game::IPhysicsFactory>(game::kPhysicsFactoryInterface);
    game::IPhysics* physics = physicsFactory != nullptr ? physicsFactory->createWorld() : nullptr;
    check(physics != nullptr && physics->initialize(), "physics.dll loaded");
    if (physics == nullptr) {
        return 1;
    }
    physics->setCharacterCollisions(true);
    game::level::buildPhysics(*physics, game::EMap::kArena);

    game::World_t server;
    game::world::reset(server, 64);
    server.physics = physics;
    const auto player = game::world::spawnPlayer(server, 1);  // x = 1.5, on the ground
    check(server.registry.valid(player), "player entity was created");

    // Gravity + ground: wait 1 second, it must stay on the ground.
    for (int i = 0; i < 64; ++i) {
        game::world::tick(server);
    }
    const auto& idle = server.registry.get<game::Transform_t>(player);
    check(std::abs(idle.position.y) < 0.05f, "settled on the ground (y ~ 0)");
    check(server.registry.get<game::Player_t>(player).onGround, "onGround true");

    // Fire (12 m from the wall): a projectile entity must appear, and be deleted when it hits the wall.
    game::UserCmd_t fire;
    fire.buttons = game::buttonBit(game::EButton::kAttack);
    game::world::enqueueCommand(server, 1, fire);
    game::world::tick(server);
    check(server.registry.view<game::Projectile_t>().size() == 1, "projectile created");
    game::world::enqueueCommand(server, 1, game::UserCmd_t{});  // release fire (the last command is repeated)
    for (int i = 0; i < 64; ++i) {
        game::world::tick(server);
    }
    check(server.registry.view<game::Projectile_t>().size() == 0, "projectile hit the wall and was deleted");

    // Walk straight forward: must stop in front of the wall z=-12 (thickness 0.6): z ~ -11.7 + radius.
    for (int i = 0; i < 64 * 5; ++i) {
        game::world::enqueueCommand(server, 1, forwardCmd());
        game::world::tick(server);
    }
    const auto& walked = server.registry.get<game::Transform_t>(player);
    check(walked.position.z < -5.0f, "walked forward");
    check(walked.position.z > -11.7f, "hit the wall, did not pass through it");

    // Replication + protobuf: server -> bytes -> client world.
    const game::WorldSnapshot_t captured = game::replication::capture(server);
    game::Envelope_t envelope;
    envelope.type = netproto::Envelope::TYPE_SNAPSHOT;
    envelope.snapshot = captured;
    const auto bytes = game::protocol::encode(envelope);
    check(bytes.has_value(), "snapshot protobuf+gzip encode");
    const auto decoded = game::protocol::decode(*bytes);
    check(decoded.has_value(), "snapshot decode");

    game::World_t client;
    game::world::reset(client, 64);
    game::replication::apply(client, decoded->snapshot);
    const auto clientPlayer = game::world::findPlayer(client, 1);
    check(clientPlayer != entt::null, "client found the player");
    const auto& clientTransform = client.registry.get<game::Transform_t>(clientPlayer);
    check(std::abs(clientTransform.position.z - walked.position.z) < 1e-4f, "client position same as server");

    // When a player leaves, it must be deleted from the client too.
    game::world::despawnPlayer(server, 1);
    game::replication::apply(client, game::replication::capture(server));
    check(game::world::findPlayer(client, 1) == entt::null, "departed player deleted on the client");

    // Player-player collision (server setting player.collisions.enabled): the player on the right walks left and stops at the one on the left.
    const auto walkIntoNeighbor = [&](uint32_t leftId, uint32_t rightId, bool collisions) {
        physics->setCharacterCollisions(collisions);
        game::World_t arena;
        game::world::reset(arena, 64);
        arena.physics = physics;
        const auto left = game::world::spawnPlayer(arena, leftId);
        const auto right = game::world::spawnPlayer(arena, rightId);
        game::UserCmd_t goLeft;
        goLeft.sideMove = -1.0f;  // yaw=0: left = -X
        for (int i = 0; i < 64 * 3; ++i) {
            game::world::enqueueCommand(arena, rightId, goLeft);
            game::world::tick(arena);
        }
        const float gap = arena.registry.get<game::Transform_t>(right).position.x - arena.registry.get<game::Transform_t>(left).position.x;
        game::world::despawnPlayer(arena, leftId);
        game::world::despawnPlayer(arena, rightId);
        return gap;
    };
    check(walkIntoNeighbor(5, 6, true) > 0.5f, "collision on: player did not pass through the other");
    check(walkIntoNeighbor(7, 8, false) < 0.0f, "collision off: player passed through the other");

    // Ray trace: capsule math (feet point at origin, radius 0.3, height 1.8).
    {
        constexpr glm::vec3 feet{0.0f};
        const auto body = game::rayCapsule({0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}, 10.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(body && std::abs(*body - 4.7f) < 1e-4f, "rayCapsule: horizontal ray at the body (4.7 m)");
        const auto dome = game::rayCapsule({0.0f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, 10.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(dome && std::abs(*dome - 3.2f) < 1e-4f, "rayCapsule: ray from above at the dome (3.2 m)");
        const auto miss = game::rayCapsule({0.5f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}, 10.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(!miss, "rayCapsule: ray passing by the side misses");
        const auto tooShort = game::rayCapsule({0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}, 3.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(!tooShort, "rayCapsule: misses if the distance is not enough");
        const auto inside = game::rayCapsule({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 10.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(inside && *inside == 0.0f, "rayCapsule: ray starting inside returns 0");
    }

    // Ray trace hit test: player 11 (x=16.5), player 12 (x=18.0). The projectile travels toward +X (yaw = pi/2).
    {
        physics->setCharacterCollisions(false);
        const auto fireAcross = [&](uint32_t shooterId, uint32_t victimId, bool wallBetween) {
            game::World_t arena;
            game::world::reset(arena, 64);
            arena.physics = physics;
            const auto shooter = game::world::spawnPlayer(arena, shooterId);
            const auto victim = game::world::spawnPlayer(arena, victimId);
            if (wallBetween) {
                const glm::vec3 mid = (arena.registry.get<game::Transform_t>(shooter).position + arena.registry.get<game::Transform_t>(victim).position) * 0.5f;
                physics->addStaticBox(mid + glm::vec3{0.0f, 1.0f, 0.0f}, {0.1f, 2.0f, 2.0f});
            }
            game::UserCmd_t fire;
            fire.yaw = 1.5707963f;
            fire.buttons = game::buttonBit(game::EButton::kAttack);
            game::world::enqueueCommand(arena, shooterId, fire);
            game::world::tick(arena);
            game::UserCmd_t idle;
            idle.yaw = fire.yaw;
            for (int i = 0; i < 10; ++i) {
                game::world::enqueueCommand(arena, shooterId, idle);
                game::world::tick(arena);
            }
            struct Result_t {
                uint32_t victimHealth;
                uint32_t shooterHealth;
                size_t projectiles;
            };
            Result_t result{arena.registry.get<game::Player_t>(victim).health, arena.registry.get<game::Player_t>(shooter).health,
                            arena.registry.view<game::Projectile_t>().size()};
            game::world::despawnPlayer(arena, shooterId);
            game::world::despawnPlayer(arena, victimId);
            return result;
        };
        const auto direct = fireAcross(11, 12, false);
        check(direct.victimHealth == 100 - game::kProjectileDamage, "projectile hit the player: health dropped");
        check(direct.shooterHealth == 100, "shooter did not hit itself");
        check(direct.projectiles == 0, "the projectile that hit was deleted");
        const auto blocked = fireAcross(21, 22, true);
        check(blocked.victimHealth == 100, "player behind the wall was not hit");
        check(blocked.projectiles == 0, "projectile that hit the wall was deleted");

        // Direct trace::line.
        game::World_t arena;
        game::world::reset(arena, 64);
        arena.physics = physics;
        const auto a = game::world::spawnPlayer(arena, 31);
        const auto b = game::world::spawnPlayer(arena, 32);
        const glm::vec3 eye = arena.registry.get<game::Transform_t>(a).position + glm::vec3{0.0f, game::kEyeHeight, 0.0f};
        const auto hit = game::trace::line(arena, {.origin = eye, .direction = {1.0f, 0.0f, 0.0f}, .maxDistance = 10.0f, .ignore = a});
        // Eye height (1.6 m) lines up with the capsule's top dome: 1.5 - sqrt(0.3^2 - 0.1^2) = 1.2172 m.
        check(hit.kind == game::trace::ETraceKind::kPlayer && hit.entity == b && std::abs(hit.distance - 1.2172f) < 1e-3f,
              "trace::line: found the player on the dome (1.217 m)");
        const auto self = game::trace::line(arena, {.origin = eye, .direction = {-1.0f, 0.0f, 0.0f}, .maxDistance = 3.0f, .ignore = a});
        check(self.kind == game::trace::ETraceKind::kNone, "trace::line: ignore and empty direction: no hit");
        game::world::despawnPlayer(arena, 31);
        game::world::despawnPlayer(arena, 32);
    }

    // Client-side prediction: "server" and "client" characters in the same physics world (collisions off).
    {
        physics->setCharacterCollisions(false);
        game::World_t srv;
        game::world::reset(srv, 64);
        srv.physics = physics;
        const auto srvPlayer = game::world::spawnPlayer(srv, 1);

        game::World_t cli;
        game::world::reset(cli, 64);
        cli.physics = physics;
        game::replication::apply(cli, game::replication::capture(srv));
        const auto self = game::world::findPlayer(cli, 1);
        game::prediction::Prediction_t state;
        state.physics = physics;

        // Client predicts 30 commands, the server has processed only the first 20.
        for (uint32_t seq = 1; seq <= 30; ++seq) {
            game::UserCmd_t cmd = forwardCmd();
            cmd.tick = seq;
            game::prediction::predict(cli, state, self, cmd);
            if (seq <= 20) {
                game::world::enqueueCommand(srv, 1, cmd);
                game::world::tick(srv);
            }
        }
        const glm::vec3 beforeApply = cli.registry.get<game::Transform_t>(self).position;
        game::replication::apply(cli, game::replication::capture(srv));
        check(cli.registry.get<game::Transform_t>(self).position.z > beforeApply.z + 0.01f, "apply pulled the local player back (to the server position)");
        game::prediction::reconcile(cli, state, self);
        const glm::vec3 afterReconcile = cli.registry.get<game::Transform_t>(self).position;
        check(state.unacked.size() == 10, "reconcile: 20 acked commands removed, 10 remain");
        check(glm::length(afterReconcile - beforeApply) < 1e-3f, "reconcile: replay arrived at the same position as the prediction");
        check(glm::length(state.errorOffset) < 1e-3f, "reconcile: no deviation, errorOffset ~ 0");

        // If the server character slides 0.5 m sideways: the visual deviation is 0.5 m, it fades in 1 s.
        const auto nudge = [&](float dx) {
            auto& transform = srv.registry.get<game::Transform_t>(srvPlayer);
            transform.position.x += dx;
            physics->setCharacterPosition(srv.registry.get<game::Collider_t>(srvPlayer).handle, transform.position);
            game::replication::apply(cli, game::replication::capture(srv));
            game::prediction::reconcile(cli, state, self);
        };
        nudge(0.5f);
        check(std::abs(glm::length(state.errorOffset) - 0.5f) < 0.05f, "small deviation: errorOffset ~ 0.5 m");
        for (int i = 0; i < 64; ++i) {
            game::prediction::decay(state, 1.0f / 64.0f);
        }
        check(glm::length(state.errorOffset) < 0.01f, "errorOffset faded in 1 s");
        nudge(5.0f);
        check(glm::length(state.errorOffset) == 0.0f, "big deviation (5 m): instant snap, errorOffset == 0");

        game::world::despawnPlayer(srv, 1);
        physics->destroyCharacter(cli.registry.get<game::Collider_t>(self).handle);
    }

    // Server command queue limit: if 10 commands arrive at once, the old ones are dropped but the jump edge is not lost.
    {
        game::World_t srv;
        game::world::reset(srv, 64);
        srv.physics = physics;
        const auto p = game::world::spawnPlayer(srv, 2);
        for (uint32_t seq = 1; seq <= 10; ++seq) {
            game::UserCmd_t cmd = forwardCmd();
            cmd.tick = seq;
            if (seq == 1) {
                cmd.buttonsPressed = game::buttonBit(game::EButton::kJump);
            }
            game::world::enqueueCommand(srv, 2, cmd);
        }
        game::world::tick(srv);
        const auto& queue = srv.registry.get<game::CommandQueue_t>(game::world::findController(srv, 2));
        check(queue.pending.size() <= game::kMaxCmdBacklog, "queue limit: pending commands <= kMaxCmdBacklog");
        check(srv.registry.get<game::Player_t>(p).lastCmdSeq == 8, "queue limit: oldest commands dropped (ack 8)");
        check(game::hasButton(queue.current.buttonsPressed, game::EButton::kJump), "queue limit: jump edge carried over from the dropped command");
        game::world::despawnPlayer(srv, 2);
    }

    // Pawn / Controller: watch, take over a bot, release. The player's pawn must be protected.
    {
        using game::EControlAction;
        game::World_t srv;
        game::world::reset(srv, 64);
        srv.physics = physics;
        const auto home = game::world::spawnPlayer(srv, 5);
        const auto other = game::world::spawnPlayer(srv, 6);
        const auto bot = game::world::spawnBot(srv, {0.0f, 0.0f, -4.0f}, 0.6f);
        const auto netIdOf = [&](entt::entity e) { return srv.registry.get<game::Replicated_t>(e).netId; };
        const auto controllerOfPlayer = game::world::findController(srv, 5);

        check(game::world::findPlayer(srv, 5) == home, "pawn: the player drives its own pawn");
        check(!game::world::handleControl(srv, 5, {.action = EControlAction::kTakeOver}), "pawn: cannot be taken over without watching");
        check(!game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(home)}), "pawn: cannot watch itself");

        check(game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(bot)}), "pawn: watching the bot accepted");
        check(game::world::spectatedNetId(srv, 5) == netIdOf(bot), "pawn: watched netId is the bot");
        game::world::tick(srv);  // ownership is synced at the end of the tick
        check(game::world::findPlayer(srv, 5) == entt::entity{entt::null}, "pawn: own pawn released while watching");

        check(game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(other)}), "pawn: watching a player accepted");
        check(!game::world::handleControl(srv, 5, {.action = EControlAction::kTakeOver}), "pawn: a player's pawn cannot be taken over");

        check(game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(bot)}), "pawn: watch the bot again");
        check(game::world::handleControl(srv, 5, {.action = EControlAction::kTakeOver}), "pawn: bot taken over");
        game::world::tick(srv);
        check(game::world::findPlayer(srv, 5) == bot, "pawn: player now drives the bot (playerId synced)");
        check(game::world::spectatedNetId(srv, 5) == 0, "pawn: watching ends on takeover");

        check(game::world::handleControl(srv, 5, {.action = EControlAction::kRelease}), "pawn: release accepted");
        game::world::tick(srv);
        check(game::world::findPlayer(srv, 5) == home, "pawn: player returned to its own pawn");
        check(game::control::controllerOf(srv.registry, bot) != entt::entity{entt::null} &&
                  srv.registry.all_of<game::AiController_t>(game::control::controllerOf(srv.registry, bot)),
              "pawn: bot returned to its AI controller");

        // If the player driving the bot leaves, the bot returns to AI.
        check(game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(bot)}) &&
                  game::world::handleControl(srv, 5, {.action = EControlAction::kTakeOver}),
              "pawn: take over again");
        game::world::despawnPlayer(srv, 5);
        check(srv.registry.all_of<game::AiController_t>(game::control::controllerOf(srv.registry, bot)), "pawn: bot returned to AI when the player left");
        (void)controllerOfPlayer;

        // Death: the bot is deleted (its AI controller too); the player's pawn is deleted but its controller stays.
        const auto botController = game::control::controllerOf(srv.registry, bot);
        srv.registry.get<game::Player_t>(bot).health = 0;
        srv.registry.get<game::Player_t>(other).health = 0;
        game::world::tick(srv);
        check(!srv.registry.valid(bot) && !srv.registry.valid(botController), "death: bot and its AI controller deleted");
        check(!srv.registry.valid(other), "death: the player's pawn deleted");
        check(game::world::findController(srv, 6) != entt::entity{entt::null}, "death: the dead player's controller remained");
        check(game::world::findPlayer(srv, 6) == entt::entity{entt::null}, "death: the dead player has no pawn");
        check(!game::world::handleControl(srv, 6, {.action = EControlAction::kRelease}), "death: a dead player cannot return to its pawn");
        game::world::despawnPlayer(srv, 6);
    }

    // Interpolation: the remote player moves with x = tick * 0.1; the local player (id 1) is never touched.
    {
        const auto snapshotAt = [](uint32_t tick, float yaw) {
            game::WorldSnapshot_t snapshot{.tick = tick, .tickRate = 64};
            for (uint32_t id = 1; id <= 2; ++id) {
                snapshot.entities.push_back({.netId = id,
                                             .transform = game::Transform_t{.position = {static_cast<float>(tick) * 0.1f, 0.0f, 0.0f}, .yaw = yaw},
                                             .player = game::Player_t{.playerId = id}});
            }
            return snapshot;
        };
        game::World_t view;
        game::world::reset(view, 64);
        game::replication::apply(view, snapshotAt(12, 0.0f));
        const auto remote = game::world::findPlayer(view, 2);
        const auto local = game::world::findPlayer(view, 1);
        const auto xOf = [&](entt::entity e) { return view.registry.get<game::Transform_t>(e).position.x; };

        game::interpolation::Interpolation_t state;
        game::interpolation::apply(view, state, 1);  // empty buffer: no jump or crash
        check(std::abs(xOf(remote) - 1.2f) < 1e-4f, "interpolation: with an empty buffer, the world stayed as it was");

        game::interpolation::push(state, snapshotAt(10, 0.0f));
        game::interpolation::push(state, snapshotAt(11, 0.0f));
        game::interpolation::push(state, snapshotAt(12, 0.0f));
        state.renderTick = 10.5;
        game::interpolation::apply(view, state, 1);
        check(std::abs(xOf(remote) - 1.05f) < 1e-4f, "interpolation: renderTick 10.5 -> x == 1.05");
        check(std::abs(xOf(local) - 1.2f) < 1e-4f, "interpolation: the local player was not touched");

        // The display time jumps to the target (big difference), then advances smoothly; it never passes the newest tick.
        state.delaySeconds = 0.05f;  // 64 Hz: ceil(3.2) = 4 ticks
        state.renderTick = 0.0;
        game::interpolation::advance(state, 64);
        check(std::abs(state.renderTick - 8.0) < 1e-9, "interpolation: jumped to the target on a big difference (12 - 4)");
        state.renderTick = 7.0;
        game::interpolation::advance(state, 64);
        check(std::abs(state.renderTick - 8.1) < 1e-9, "interpolation: approached smoothly on a small difference");
        state.delaySeconds = 0.0f;
        state.enabled = false;
        game::interpolation::advance(state, 64);
        check(state.renderTick == 12.0, "interpolation: newest tick when disabled");
        game::interpolation::apply(view, state, 1);
        check(std::abs(xOf(remote) - 1.2f) < 1e-4f, "interpolation: renderTick at the newest tick: no extrapolation");

        // Angle wrap: 3.1 -> -3.1 goes the short way (over pi).
        const float mid = game::interpolation::lerpAngle(3.1f, -3.1f, 0.5f);
        check(std::abs(std::abs(mid) - std::numbers::pi_v<float>) < 0.01f, "lerpAngle: does not turn the wrong way at the wrap (~ pi)");

        // Projectile: position at a fractional tick; before spawn it is at the muzzle point.
        game::Projectile_t bullet{.spawnTick = 10, .origin = {0.0f, 0.0f, 0.0f}, .velocity = {64.0f, 0.0f, 0.0f}};
        check(std::abs(game::systems::projectilePosition(bullet, 10.5, 1.0f / 64.0f).x - 0.5f) < 1e-4f, "projectilePosition: fractional tick (10.5 -> 0.5 m)");
        check(game::systems::projectilePosition(bullet, 5.0, 1.0f / 64.0f).x == 0.0f, "projectilePosition: muzzle point before spawn");
    }

    // Animated entity (server logic): turns toward the nearby player, waves when close, stops when it moves away;
    // the state is replicated to the client (not the pose, only state + startTick).
    {
        game::World_t arena;
        game::world::reset(arena, 64);
        // No physics: the player's Transform is positioned directly (the capsule does not write back every tick).
        const auto robot = game::world::spawnAnimated(arena, game::EModel::kRobot, {0.0f, 0.0f, 0.0f});
        const auto visitor = game::world::spawnPlayer(arena, 20);  // x = 30: outside the robot, far away (> kLookRadius)
        const auto stateOf = [&] { return arena.registry.get<game::Animator_t>(robot).state; };
        for (int i = 0; i < 64; ++i) {
            game::world::tick(arena);
        }
        check(stateOf() == game::EAnimState::kIdle, "robot: idle for a player far away");
        check(arena.registry.get<game::Transform_t>(robot).yaw == 0.0f, "robot: does not turn toward a player outside its range");

        const auto setVisitorX = [&](float x) {
            arena.registry.get<game::Transform_t>(visitor).position.x = x;
            game::world::tick(arena);
        };
        for (int i = 0; i < 128; ++i) {
            setVisitorX(-8.0f);  // 8 m: sees it, does not wave
        }
        check(stateOf() == game::EAnimState::kIdle, "robot: idle at 8 m");
        check(arena.registry.get<game::Transform_t>(robot).yaw > 1.0f, "robot: turned toward the player (player at -X: yaw ~ pi/2)");

        setVisitorX(-3.0f);
        const uint32_t waveStart = arena.tick - 1;
        check(stateOf() == game::EAnimState::kGreet, "robot: greets at 3 m");
        const auto& animator = arena.registry.get<game::Animator_t>(robot);
        check(animator.startTick == waveStart && animator.prevState == game::EAnimState::kIdle, "robot: startTick and prevState were written");

        setVisitorX(-5.5f);  // enter 5 m, exit 6.5 m: hysteresis, still waves
        check(stateOf() == game::EAnimState::kGreet, "robot: hysteresis (still greeting at 5.5 m)");
        setVisitorX(-7.0f);
        check(stateOf() == game::EAnimState::kIdle, "robot: returned to idle at 7 m");

        // Replication: the robot appears on the client with Animator_t + Transform_t.
        game::Envelope_t animEnvelope;
        animEnvelope.type = netproto::Envelope::TYPE_SNAPSHOT;
        animEnvelope.snapshot = game::replication::capture(arena);
        const auto animBytes = game::protocol::encode(animEnvelope);
        const auto animDecoded = game::protocol::decode(*animBytes);
        game::World_t animClient;
        game::world::reset(animClient, 64);
        game::replication::apply(animClient, animDecoded->snapshot);
        const auto replicaRobot = animClient.registry.view<game::Animator_t>();
        check(replicaRobot.size() == 1, "robot replicated to the client");
        if (replicaRobot.size() == 1) {
            const auto& replica = animClient.registry.get<game::Animator_t>(replicaRobot.front());
            const auto& source = arena.registry.get<game::Animator_t>(robot);
            check(replica.state == source.state && replica.startTick == source.startTick && replica.prevState == source.prevState &&
                      replica.prevStartTick == source.prevStartTick,
                  "robot: Animator_t carried over exactly via protobuf");
        }
    }

    physics->shutdown();
    std::cout << (g_failures == 0 ? "ALL TESTS PASSED\n" : "SOME TESTS FAILED\n");
    return g_failures == 0 ? 0 : 1;
}
