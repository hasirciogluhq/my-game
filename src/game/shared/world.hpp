#pragma once

#include "engine/engine/core/types.hpp"
#include "engine/interface/i_physics.hpp"
#include "game/shared/components.hpp"

#include <cstdint>
#include <unordered_map>

#include <entt/entity/registry.hpp>

namespace game {

// Oyun dünyası: tüm entity'ler registry'de. Server ve client aynı tipi
// kullanır.
struct World_t {
  entt::registry registry;
  IPhysics *physics = nullptr; // client'ta sadece yerel tahmin için
  uint32_t tick = 0;
  uint32_t tickRate = kDefaultTickRate;
  uint32_t nextNetId = 1;
  std::unordered_map<uint32_t, entt::entity> byNetId;
};

namespace world {

void reset(World_t &world, uint32_t tickRate);
[[nodiscard]] float tickDelta(const World_t &world);

// Replicated entity'ler (netId ile bulunur).
[[nodiscard]] entt::entity createReplicated(World_t &world); // yeni netId atar
[[nodiscard]] entt::entity findOrCreateReplicated(World_t &world,
                                                  uint32_t netId);
void destroy(World_t &world, entt::entity entity);

[[nodiscard]] entt::entity findByNetId(const World_t &world,
                                       uint32_t netId); // entt::null olabilir

// Oyuncular (server). Oyuncu = PlayerController_t entity'si + onun sürdüğü
// karakter pawn'ı. spawnPlayer pawn'ı döndürür; oyuncu varsa kendi pawn'ını
// (homePawn) döndürür.
entt::entity spawnPlayer(World_t &world, uint32_t playerId);
void despawnPlayer(World_t &world, uint32_t playerId);
// Karakter pawn'ını (fizik kapsülüyle) siler. Controller'ı kalır (oyuncu ölünce bağlantı ve controller durur).
void killPawn(World_t &world, entt::entity pawn);
// Bot: AiController_t + karakter pawn'ı. Pawn'ı döndürür.
entt::entity spawnBot(World_t &world, const glm::vec3 &position);
// playerId'li oyuncunun şu an SÜRDÜĞÜ pawn (client'ta replicate edilen
// Player_t'den bulunur). entt::null olabilir.
[[nodiscard]] entt::entity findPlayer(const World_t &world, uint32_t playerId);
// playerId'li oyuncunun controller'ı (sadece server'da var). entt::null
// olabilir.
[[nodiscard]] entt::entity findController(const World_t &world,
                                          uint32_t playerId);
void enqueueCommand(World_t &world, uint32_t playerId, const UserCmd_t &cmd);
// Animasyonlu prop (server): replicate edilir, durumunu systems::animation seçer. Controller'sız, çarpışmasız.
entt::entity spawnAnimated(World_t &world, EModel model,
                           const glm::vec3 &position, float yaw = 0.0f);

} // namespace world
} // namespace game
