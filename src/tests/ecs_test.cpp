// Başsız entegrasyon testi: physics.dll + ECS sistemleri + replikasyon + protobuf.
// Çalıştır: xmake run ecs_test
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
    cmd.forwardMove = 1.0f;  // yaw=0: -Z'ye yürü
    return cmd;
}

}  // namespace

int main(int argc, char** argv) {
    (void)argc;
    // Motor modülleri out/.../engine altında, test exe'si out/.../game altında.
    const std::filesystem::path engineDir = std::filesystem::absolute(argv[0]).parent_path().parent_path() / "engine";
    game::CModule physicsModule;
    if (!physicsModule.load(engineDir / game::moduleFileName("physics"))) {
        return 1;
    }
    auto* physics = physicsModule.getInterface<game::IPhysics>(game::kPhysicsInterface);
    check(physics != nullptr && physics->initialize(), "physics.dll yüklendi");
    if (physics == nullptr) {
        return 1;
    }
    physics->setCharacterCollisions(true);
    game::level::buildPhysics(*physics);

    game::World_t server;
    game::world::reset(server, 64);
    server.physics = physics;
    const auto player = game::world::spawnPlayer(server, 1);  // x = 1.5, zeminde
    check(server.registry.valid(player), "oyuncu entity'si oluştu");

    // Yerçekimi + zemin: 1 saniye bekle, zeminde kalmalı.
    for (int i = 0; i < 64; ++i) {
        game::world::tick(server);
    }
    const auto& idle = server.registry.get<game::Transform_t>(player);
    check(std::abs(idle.position.y) < 0.05f, "zemine oturdu (y ~ 0)");
    check(server.registry.get<game::Player_t>(player).onGround, "onGround true");

    // Ateş et (duvara 12 m uzakta): mermi entity'si oluşmalı, duvara çarpınca silinmeli.
    game::UserCmd_t fire;
    fire.buttons = game::buttonBit(game::EButton::kAttack);
    game::world::enqueueCommand(server, 1, fire);
    game::world::tick(server);
    check(server.registry.view<game::Projectile_t>().size() == 1, "mermi yaratıldı");
    game::world::enqueueCommand(server, 1, game::UserCmd_t{});  // ateşi bırak (son komut tekrarlanır)
    for (int i = 0; i < 64; ++i) {
        game::world::tick(server);
    }
    check(server.registry.view<game::Projectile_t>().size() == 0, "mermi duvara çarpıp silindi");

    // Düz ileri yürü: duvar z=-12 (kalınlık 0.6) önünde durmalı: z ~ -11.7 + yarıçap.
    for (int i = 0; i < 64 * 5; ++i) {
        game::world::enqueueCommand(server, 1, forwardCmd());
        game::world::tick(server);
    }
    const auto& walked = server.registry.get<game::Transform_t>(player);
    check(walked.position.z < -5.0f, "ileri yürüdü");
    check(walked.position.z > -11.7f, "duvara çarptı, içinden geçmedi");

    // Replikasyon + protobuf: server -> bytes -> client world.
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
    check(clientPlayer != entt::null, "client oyuncuyu buldu");
    const auto& clientTransform = client.registry.get<game::Transform_t>(clientPlayer);
    check(std::abs(clientTransform.position.z - walked.position.z) < 1e-4f, "client konumu server ile aynı");

    // Oyuncu ayrılınca client'tan da silinmeli.
    game::world::despawnPlayer(server, 1);
    game::replication::apply(client, game::replication::capture(server));
    check(game::world::findPlayer(client, 1) == entt::null, "ayrılan oyuncu client'ta silindi");

    // Oyuncu-oyuncu çarpışması (server ayarı player.collisions.enabled): sağdaki oyuncu sola yürüyüp soldakine dayanır.
    const auto walkIntoNeighbor = [&](uint32_t leftId, uint32_t rightId, bool collisions) {
        physics->setCharacterCollisions(collisions);
        game::World_t arena;
        game::world::reset(arena, 64);
        arena.physics = physics;
        const auto left = game::world::spawnPlayer(arena, leftId);
        const auto right = game::world::spawnPlayer(arena, rightId);
        game::UserCmd_t goLeft;
        goLeft.sideMove = -1.0f;  // yaw=0: sola = -X
        for (int i = 0; i < 64 * 3; ++i) {
            game::world::enqueueCommand(arena, rightId, goLeft);
            game::world::tick(arena);
        }
        const float gap = arena.registry.get<game::Transform_t>(right).position.x - arena.registry.get<game::Transform_t>(left).position.x;
        game::world::despawnPlayer(arena, leftId);
        game::world::despawnPlayer(arena, rightId);
        return gap;
    };
    check(walkIntoNeighbor(5, 6, true) > 0.5f, "çarpışma açık: oyuncu diğerinin içinden geçmedi");
    check(walkIntoNeighbor(7, 8, false) < 0.0f, "çarpışma kapalı: oyuncu diğerinin içinden geçti");

    // Ray trace: kapsül matematiği (ayak noktası orijin, yarıçap 0.3, boy 1.8).
    {
        constexpr glm::vec3 feet{0.0f};
        const auto body = game::rayCapsule({0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}, 10.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(body && std::abs(*body - 4.7f) < 1e-4f, "rayCapsule: gövdeye yatay ışın (4.7 m)");
        const auto dome = game::rayCapsule({0.0f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, 10.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(dome && std::abs(*dome - 3.2f) < 1e-4f, "rayCapsule: tepeye yukarıdan ışın (3.2 m)");
        const auto miss = game::rayCapsule({0.5f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}, 10.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(!miss, "rayCapsule: yandan geçen ışın ıskalar");
        const auto tooShort = game::rayCapsule({0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}, 3.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(!tooShort, "rayCapsule: mesafe yetmezse ıskalar");
        const auto inside = game::rayCapsule({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 10.0f, feet, game::kCharacterRadius, game::kCharacterHeight);
        check(inside && *inside == 0.0f, "rayCapsule: içeriden başlayan ışın 0 döner");
    }

    // Ray trace hit test: oyuncu 11 (x=16.5), oyuncu 12 (x=18.0). Mermi +X'e gider (yaw = pi/2).
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
        check(direct.victimHealth == 100 - game::kProjectileDamage, "mermi oyuncuyu vurdu: can düştü");
        check(direct.shooterHealth == 100, "ateş eden kendini vurmadı");
        check(direct.projectiles == 0, "isabet eden mermi silindi");
        const auto blocked = fireAcross(21, 22, true);
        check(blocked.victimHealth == 100, "duvarın arkasındaki oyuncu vurulmadı");
        check(blocked.projectiles == 0, "duvara çarpan mermi silindi");

        // Doğrudan trace::line.
        game::World_t arena;
        game::world::reset(arena, 64);
        arena.physics = physics;
        const auto a = game::world::spawnPlayer(arena, 31);
        const auto b = game::world::spawnPlayer(arena, 32);
        const glm::vec3 eye = arena.registry.get<game::Transform_t>(a).position + glm::vec3{0.0f, game::kEyeHeight, 0.0f};
        const auto hit = game::trace::line(arena, {.origin = eye, .direction = {1.0f, 0.0f, 0.0f}, .maxDistance = 10.0f, .ignore = a});
        // Göz yüksekliği (1.6 m) kapsülün üst kubbesine denk gelir: 1.5 - sqrt(0.3^2 - 0.1^2) = 1.2172 m.
        check(hit.kind == game::trace::ETraceKind::kPlayer && hit.entity == b && std::abs(hit.distance - 1.2172f) < 1e-3f,
              "trace::line: oyuncuyu kubbede buldu (1.217 m)");
        const auto self = game::trace::line(arena, {.origin = eye, .direction = {-1.0f, 0.0f, 0.0f}, .maxDistance = 3.0f, .ignore = a});
        check(self.kind == game::trace::ETraceKind::kNone, "trace::line: ignore ve boş yön: isabet yok");
        game::world::despawnPlayer(arena, 31);
        game::world::despawnPlayer(arena, 32);
    }

    // Client-side prediction: aynı fizik dünyasında "server" ve "client" karakterleri (çarpışma kapalı).
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

        // Client 30 komut tahmin eder, server yalnızca ilk 20'sini işlemiş.
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
        check(cli.registry.get<game::Transform_t>(self).position.z > beforeApply.z + 0.01f, "apply yerel oyuncuyu geriye (server konumuna) çekti");
        game::prediction::reconcile(cli, state, self);
        const glm::vec3 afterReconcile = cli.registry.get<game::Transform_t>(self).position;
        check(state.unacked.size() == 10, "reconcile: ack edilen 20 komut atıldı, 10 kaldı");
        check(glm::length(afterReconcile - beforeApply) < 1e-3f, "reconcile: yeniden oynatma tahminle aynı konuma vardı");
        check(glm::length(state.errorOffset) < 1e-3f, "reconcile: sapma yok, errorOffset ~ 0");

        // Server karakteri 0.5 m yana kayarsa: görsel sapma 0.5 m, 1 sn'de söner.
        const auto nudge = [&](float dx) {
            auto& transform = srv.registry.get<game::Transform_t>(srvPlayer);
            transform.position.x += dx;
            physics->setCharacterPosition(srv.registry.get<game::Collider_t>(srvPlayer).handle, transform.position);
            game::replication::apply(cli, game::replication::capture(srv));
            game::prediction::reconcile(cli, state, self);
        };
        nudge(0.5f);
        check(std::abs(glm::length(state.errorOffset) - 0.5f) < 0.05f, "küçük sapma: errorOffset ~ 0.5 m");
        for (int i = 0; i < 64; ++i) {
            game::prediction::decay(state, 1.0f / 64.0f);
        }
        check(glm::length(state.errorOffset) < 0.01f, "errorOffset 1 sn'de söndü");
        nudge(5.0f);
        check(glm::length(state.errorOffset) == 0.0f, "büyük sapma (5 m): anında snap, errorOffset == 0");

        game::world::despawnPlayer(srv, 1);
        physics->destroyCharacter(cli.registry.get<game::Collider_t>(self).handle);
    }

    // Server komut kuyruğu sınırı: 10 komut birden gelirse eskiler atılır ama zıplama kenarı kaybolmaz.
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
        check(queue.pending.size() <= game::kMaxCmdBacklog, "kuyruk sınırı: bekleyen komut <= kMaxCmdBacklog");
        check(srv.registry.get<game::Player_t>(p).lastCmdSeq == 8, "kuyruk sınırı: en eski komutlar atıldı (ack 8)");
        check(game::hasButton(queue.current.buttonsPressed, game::EButton::kJump), "kuyruk sınırı: zıplama kenarı atılan komuttan taşındı");
        game::world::despawnPlayer(srv, 2);
    }

    // Pawn / Controller: izle, bot'u devral, bırak. Oyuncunun pawn'ı korumalı.
    {
        using game::EControlAction;
        game::World_t srv;
        game::world::reset(srv, 64);
        srv.physics = physics;
        const auto home = game::world::spawnPlayer(srv, 5);
        const auto other = game::world::spawnPlayer(srv, 6);
        const auto bot = game::world::spawnBot(srv, {0.0f, 0.0f, -4.0f});
        const auto netIdOf = [&](entt::entity e) { return srv.registry.get<game::Replicated_t>(e).netId; };
        const auto controllerOfPlayer = game::world::findController(srv, 5);

        check(game::world::findPlayer(srv, 5) == home, "pawn: oyuncu kendi pawn'ını sürüyor");
        check(!game::world::handleControl(srv, 5, {.action = EControlAction::kTakeOver}), "pawn: izlemeden devralınamaz");
        check(!game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(home)}), "pawn: kendini izleyemez");

        check(game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(bot)}), "pawn: bot'u izleme kabul");
        check(game::world::spectatedNetId(srv, 5) == netIdOf(bot), "pawn: izlenen netId bot");
        game::world::tick(srv);  // sahiplik tick sonunda eşitlenir
        check(game::world::findPlayer(srv, 5) == entt::entity{entt::null}, "pawn: izlerken kendi pawn'ı bırakılır");

        check(game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(other)}), "pawn: oyuncuyu izleme kabul");
        check(!game::world::handleControl(srv, 5, {.action = EControlAction::kTakeOver}), "pawn: oyuncunun pawn'ı devralınamaz");

        check(game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(bot)}), "pawn: tekrar bot'u izle");
        check(game::world::handleControl(srv, 5, {.action = EControlAction::kTakeOver}), "pawn: bot devralındı");
        game::world::tick(srv);
        check(game::world::findPlayer(srv, 5) == bot, "pawn: oyuncu artık bot'u sürüyor (playerId eşitlendi)");
        check(game::world::spectatedNetId(srv, 5) == 0, "pawn: devralınca izleme biter");

        check(game::world::handleControl(srv, 5, {.action = EControlAction::kRelease}), "pawn: bırakma kabul");
        game::world::tick(srv);
        check(game::world::findPlayer(srv, 5) == home, "pawn: oyuncu kendi pawn'ına döndü");
        check(game::control::controllerOf(srv.registry, bot) != entt::entity{entt::null} &&
                  srv.registry.all_of<game::AiController_t>(game::control::controllerOf(srv.registry, bot)),
              "pawn: bot AI controller'ına geri döndü");

        // Bot'u süren oyuncu çıkarsa bot AI'a döner.
        check(game::world::handleControl(srv, 5, {EControlAction::kSpectate, netIdOf(bot)}) &&
                  game::world::handleControl(srv, 5, {.action = EControlAction::kTakeOver}),
              "pawn: tekrar devralma");
        game::world::despawnPlayer(srv, 5);
        check(srv.registry.all_of<game::AiController_t>(game::control::controllerOf(srv.registry, bot)), "pawn: oyuncu çıkınca bot AI'a döndü");
        (void)controllerOfPlayer;

        // Ölüm: bot silinir (AI controller'ı da), oyuncunun pawn'ı silinir ama controller'ı kalır.
        const auto botController = game::control::controllerOf(srv.registry, bot);
        srv.registry.get<game::Player_t>(bot).health = 0;
        srv.registry.get<game::Player_t>(other).health = 0;
        game::world::tick(srv);
        check(!srv.registry.valid(bot) && !srv.registry.valid(botController), "ölüm: bot ve AI controller'ı silindi");
        check(!srv.registry.valid(other), "ölüm: oyuncunun pawn'ı silindi");
        check(game::world::findController(srv, 6) != entt::entity{entt::null}, "ölüm: ölen oyuncunun controller'ı kaldı");
        check(game::world::findPlayer(srv, 6) == entt::entity{entt::null}, "ölüm: ölen oyuncunun pawn'ı yok");
        check(!game::world::handleControl(srv, 6, {.action = EControlAction::kRelease}), "ölüm: ölü oyuncu pawn'ına dönemez");
        game::world::despawnPlayer(srv, 6);
    }

    // Interpolation: uzaktaki oyuncu x = tick * 0.1 ile gider; yerel oyuncu (id 1) hiç dokunulmaz.
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
        game::interpolation::apply(view, state, 1);  // boş tampon: sıçrama/çökme yok
        check(std::abs(xOf(remote) - 1.2f) < 1e-4f, "interpolation: boş tamponda dünya aynen kaldı");

        game::interpolation::push(state, snapshotAt(10, 0.0f));
        game::interpolation::push(state, snapshotAt(11, 0.0f));
        game::interpolation::push(state, snapshotAt(12, 0.0f));
        state.renderTick = 10.5;
        game::interpolation::apply(view, state, 1);
        check(std::abs(xOf(remote) - 1.05f) < 1e-4f, "interpolation: renderTick 10.5 -> x == 1.05");
        check(std::abs(xOf(local) - 1.2f) < 1e-4f, "interpolation: yerel oyuncuya dokunulmadı");

        // Gösterim zamanı hedefe sıçrar (büyük fark), sonra yumuşak ilerler, en yeni tick'i geçmez.
        state.delaySeconds = 0.05f;  // 64 Hz'de ceil(3.2) = 4 tick
        state.renderTick = 0.0;
        game::interpolation::advance(state, 64);
        check(std::abs(state.renderTick - 8.0) < 1e-9, "interpolation: büyük farkta hedefe sıçradı (12 - 4)");
        state.renderTick = 7.0;
        game::interpolation::advance(state, 64);
        check(std::abs(state.renderTick - 8.1) < 1e-9, "interpolation: küçük farkta yumuşak yaklaştı");
        state.delaySeconds = 0.0f;
        state.enabled = false;
        game::interpolation::advance(state, 64);
        check(state.renderTick == 12.0, "interpolation: kapalıyken en yeni tick");
        game::interpolation::apply(view, state, 1);
        check(std::abs(xOf(remote) - 1.2f) < 1e-4f, "interpolation: renderTick en yeni tick'te: extrapolation yok");

        // Açı sarması: 3.1 -> -3.1 kısa yoldan (pi üzerinden) gider.
        const float mid = game::interpolation::lerpAngle(3.1f, -3.1f, 0.5f);
        check(std::abs(std::abs(mid) - std::numbers::pi_v<float>) < 0.01f, "lerpAngle: sarmada ters yöne dönmez (~ pi)");

        // Mermi: kesirli tick ile konum; doğumdan önce çıkış noktasında.
        game::Projectile_t bullet{.spawnTick = 10, .origin = {0.0f, 0.0f, 0.0f}, .velocity = {64.0f, 0.0f, 0.0f}};
        check(std::abs(game::systems::projectilePosition(bullet, 10.5, 1.0f / 64.0f).x - 0.5f) < 1e-4f, "projectilePosition: kesirli tick (10.5 -> 0.5 m)");
        check(game::systems::projectilePosition(bullet, 5.0, 1.0f / 64.0f).x == 0.0f, "projectilePosition: doğumdan önce çıkış noktası");
    }

    // Animasyonlu entity (server mantığı): uzaktaki oyuncuya döner, yaklaşınca el sallar, uzaklaşınca bırakır;
    // state client'a replicate edilir (poz değil, sadece state + startTick).
    {
        game::World_t arena;
        game::world::reset(arena, 64);
        // Fizik yok: oyuncu Transform'u doğrudan konumlandırılır (kapsül her tick geri yazmaz).
        const auto robot = game::world::spawnAnimated(arena, game::EModel::kRobot, {0.0f, 0.0f, 0.0f});
        const auto visitor = game::world::spawnPlayer(arena, 20);  // x = 30: robotun dışında, uzakta (> kLookRadius)
        const auto stateOf = [&] { return arena.registry.get<game::Animator_t>(robot).state; };
        for (int i = 0; i < 64; ++i) {
            game::world::tick(arena);
        }
        check(stateOf() == game::EAnimState::kIdle, "robot: uzaktaki oyuncuya idle");
        check(arena.registry.get<game::Transform_t>(robot).yaw == 0.0f, "robot: menzil dışındaki oyuncuya dönmez");

        const auto setVisitorX = [&](float x) {
            arena.registry.get<game::Transform_t>(visitor).position.x = x;
            game::world::tick(arena);
        };
        for (int i = 0; i < 128; ++i) {
            setVisitorX(-8.0f);  // 8 m: görür ama el sallamaz
        }
        check(stateOf() == game::EAnimState::kIdle, "robot: 8 m'de idle");
        check(arena.registry.get<game::Transform_t>(robot).yaw > 1.0f, "robot: oyuncuya doğru döndü (-X'teki oyuncu: yaw ~ pi/2)");

        setVisitorX(-3.0f);
        const uint32_t waveStart = arena.tick - 1;
        check(stateOf() == game::EAnimState::kGreet, "robot: 3 m'de selamlıyor");
        const auto& animator = arena.registry.get<game::Animator_t>(robot);
        check(animator.startTick == waveStart && animator.prevState == game::EAnimState::kIdle, "robot: startTick ve prevState yazıldı");

        setVisitorX(-5.5f);  // giriş 5 m, çıkış 6.5 m: histerezis, hâlâ el sallar
        check(stateOf() == game::EAnimState::kGreet, "robot: histerezis (5.5 m'de hâlâ selamlıyor)");
        setVisitorX(-7.0f);
        check(stateOf() == game::EAnimState::kIdle, "robot: 7 m'de idle'a döndü");

        // Replikasyon: robot client'ta Animator_t + Transform_t ile belirir.
        game::Envelope_t animEnvelope;
        animEnvelope.type = netproto::Envelope::TYPE_SNAPSHOT;
        animEnvelope.snapshot = game::replication::capture(arena);
        const auto animBytes = game::protocol::encode(animEnvelope);
        const auto animDecoded = game::protocol::decode(*animBytes);
        game::World_t animClient;
        game::world::reset(animClient, 64);
        game::replication::apply(animClient, animDecoded->snapshot);
        const auto replicaRobot = animClient.registry.view<game::Animator_t>();
        check(replicaRobot.size() == 1, "robot client'a replicate edildi");
        if (replicaRobot.size() == 1) {
            const auto& replica = animClient.registry.get<game::Animator_t>(replicaRobot.front());
            const auto& source = arena.registry.get<game::Animator_t>(robot);
            check(replica.state == source.state && replica.startTick == source.startTick && replica.prevState == source.prevState &&
                      replica.prevStartTick == source.prevStartTick,
                  "robot: Animator_t protobuf ile aynen taşındı");
        }
    }

    physics->shutdown();
    std::cout << (g_failures == 0 ? "TUM TESTLER GECTI\n" : "BAZI TESTLER BASARISIZ\n");
    return g_failures == 0 ? 0 : 1;
}
