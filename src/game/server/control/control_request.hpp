#pragma once

#include "game/shared/world.hpp"

namespace game::world {

// Client'ın kontrol isteği (izle / bot'u devral / bırak). Kural dışıysa false döner, hiçbir şey değişmez.
bool handleControl(World_t& world, uint32_t playerId, const ControlRequest_t& request);
// Oyuncunun izlediği pawn'ın netId'si; izlemiyorsa 0.
[[nodiscard]] uint32_t spectatedNetId(const World_t& world, uint32_t playerId);

}  // namespace game::world
