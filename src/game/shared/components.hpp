#pragma once

#include "game/shared/game_types.hpp"

#include <cstdint>
#include <deque>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

namespace game {

// Component'ler sadece veri tutar (ECS). Mantık game/shared ve game/server içindeki sistem fonksiyonlarında.

struct Transform_t {
    glm::vec3 position{0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct Velocity_t {
    glm::vec3 linear{0.0f};
};

// Oyuncunun (ya da bot'un) sürdüğü karakter pawn'ı. Pawn/Controller bağı motorda (engine/ecs/control.hpp).
struct Player_t {
    // Bu pawn'ı şu an süren oyuncunun id'si; kimse (ya da bot) sürüyorsa 0. Her tick controller'dan eşitlenir.
    uint32_t playerId = 0;
    uint32_t health = 100;
    bool onGround = true;
    // Server'ın işlediği son komutun sıra numarası (UserCmd_t::tick). Client prediction bununla ack eder.
    uint32_t lastCmdSeq = 0;
};

// Mermi konumu her tick güncellenmez: pos(t) = origin + velocity * (t - spawnTick) * dt.
struct Projectile_t {
    uint32_t ownerId = 0;  // ateş eden pawn'ın netId'si
    uint32_t spawnTick = 0;
    glm::vec3 origin{0.0f};
    glm::vec3 velocity{0.0f};
};

// Server'ın seçtiği animasyon durumu. Client klip adını/süresini bilmez, state'i kendi modelindeki klibe eşler.
// kGreet: yakındaki oyuncuyu selamlama animasyonu; hangi klip olduğunu modelin tanımı belirler
// (robot: el sallama, dancer: breakdance). Server sadece durumu seçer.
enum class EAnimState : uint32_t {
    kIdle,
    kGreet,
};

enum class EModel : uint32_t {
    kRobot,
    kDancer,
};

// Animasyonlu (iskeletli) entity. Pozu server her tick güncellemez: sadece durum değişince yazar.
// Client pozu hesaplar: klip zamanı = (gösterim tick'i - startTick) * dt. Durum değişince önceki durumdan
// kAnimBlendSeconds (client/scene/scene.hpp) boyunca geçiş yapılır (prevState hâlâ prevStartTick'ten itibaren oynamaya devam eder).
struct Animator_t {
    EModel model = EModel::kRobot;
    EAnimState state = EAnimState::kIdle;
    uint32_t startTick = 0;
    EAnimState prevState = EAnimState::kIdle;
    uint32_t prevStartTick = 0;
};

// Ağ üzerinden kopyalanan entity'ler bunu taşır. netId server'da atanır, client'ta aynen kullanılır.
struct Replicated_t {
    uint32_t netId = 0;
};

// Fizik motorundaki karakter kapsülü (sadece fizik açıkken, yani server'da).
struct Collider_t {
    uint32_t handle = 0;
};

// Server'da pawn'ın silahı (replicate edilmez). Cooldown pawn'da kalır, controller değişince sıfırlanmaz.
struct Weapon_t {
    uint32_t nextFireTick = 0;
};

// Oyuncu controller'ı (controller entity'si, replicate edilmez). homePawn: oyuncunun kendi karakteri;
// bot'u bırakınca (release) buraya döner.
struct PlayerController_t {
    uint32_t playerId = 0;
    entt::entity homePawn = entt::null;
};

// Bot controller'ı: basit devriye (ileri yürü, yavaşça dön).
struct AiController_t {
    float turnRate = 0.6f;  // rad/sn
    float yaw = 0.0f;
};

// Controller'a gelen komutlar (oyuncu: ağdan, bot: AI). current: bu tick pawn'a uygulanan komut.
struct CommandQueue_t {
    std::deque<UserCmd_t> pending;
    UserCmd_t current;
};

}  // namespace game
