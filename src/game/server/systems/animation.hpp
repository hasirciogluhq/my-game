#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

inline constexpr float kGreetEnterRadius = 5.0f;  // bu mesafeye giren oyuncuya el sallar
inline constexpr float kGreetExitRadius = 6.5f;   // bu mesafeden uzaklaşınca bırakır (titremeyi önler)
inline constexpr float kLookRadius = 12.0f;       // en yakın oyuncuya bu mesafeden itibaren döner
inline constexpr float kTurnSpeed = 3.0f;         // rad/sn

// Animasyonlu entity'lerin davranışı (server): en yakın canlı oyuncuya doğru döner, yakınındaysa el sallar.
// Durum değişince Animator_t'ye yeni state + startTick yazar; poz client'ta bu ikiliden hesaplanır.
void animation(World_t& world);

}  // namespace game::systems
