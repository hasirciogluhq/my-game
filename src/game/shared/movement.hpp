#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Tek oyuncu, tek komut: server hareketi ve client prediction aynı kodu kullanır.
void movePlayer(World_t& world, entt::entity entity, const UserCmd_t& cmd);

// Client'ta her frame: mermi Transform'unu projection formülünden hesaplar.
// atTick kesirli olabilir (interpolation'ın gösterim zamanı); server world.tick verir.
void projectileTransforms(World_t& world, double atTick);

// pos(t) = origin + velocity * (t - spawnTick) * dt   (t < spawnTick ise çıkış noktası)
[[nodiscard]] glm::vec3 projectilePosition(const Projectile_t& projectile, double atTick, float tickDelta);

// Bakış yönü: pitch yukarı/aşağı, yaw yatay düzlemde. yaw=0 iken -Z.
[[nodiscard]] glm::vec3 lookDirection(const Transform_t& transform);

}  // namespace game::systems
