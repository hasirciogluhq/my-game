#pragma once

#include "game/shared/world.hpp"

#include <deque>
#include <optional>

#include <glm/vec3.hpp>

// Client-side prediction (sadece yerel oyuncunun hareketi). Oyun mantığı: engine/ değil game/.
// Akış: her client tick `predict` (aynı movePlayer kodu) -> snapshot gelince replication::apply
// yerel entity'yi server değerine ezer -> hemen ardından `reconcile` ack edilmeyen komutları yeniden oynatır.
namespace game::prediction {

inline constexpr size_t kMaxUnacked = 256;
inline constexpr float kSnapDistance = 2.0f;   // bundan büyük sapma anında düzeltilir (ışınlanma)
inline constexpr float kSmoothTime = 0.1f;     // errorOffset sönüm zaman sabiti (saniye)

struct Prediction_t {
    std::deque<UserCmd_t> unacked;       // server'ın henüz ack etmediği komutlar
    std::optional<glm::vec3> predicted;  // son tahmin edilen ayak konumu (henüz tahmin yoksa boş)
    glm::vec3 errorOffset{0.0f};         // görsel düzeltme: kamera/çizim konuma eklenir, zamanla söner
    bool enabled = true;                 // cl_predict
    IPhysics* physics = nullptr;      // client'ın kendi fizik örneği (ışınlama için); world.physics ile aynı nesne
};

// Yerel oyuncuya fizik karakteri verir (client'ta fizik varsa). Birden çok kez çağrılabilir.
void attach(World_t& world, entt::entity self);

// Bu tick'in komutunu yerel oyuncuya uygular ve unacked'a ekler.
void predict(World_t& world, Prediction_t& state, entt::entity self, const UserCmd_t& cmd);

// replication::apply'dan HEMEN SONRA çağrılır: ack edilenleri at, kalanları server konumundan yeniden oynat.
void reconcile(World_t& world, Prediction_t& state, entt::entity self);

// errorOffset'i sönümler.
void decay(Prediction_t& state, float dt);

// Tahmin geçmişini unutur (cl_predict değişince / yeniden bağlanınca).
void clear(Prediction_t& state);

}  // namespace game::prediction
