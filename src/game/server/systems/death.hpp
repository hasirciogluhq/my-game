#pragma once

#include "game/shared/world.hpp"

namespace game::systems {

// Canı 0 olan karakterler silinir (despawn). Bot ölünce AI controller'ı da gider. Oyuncu ölünce controller'ı
// (bağlantısı) kalır ama pawn'ı olmaz; client bunu "pawn yok" diye görür ve ölüm kamerasına geçer.
// Ölen pawn'ı izleyen controller'ların izleme hedefi temizlenir.
void death(World_t& world);

}  // namespace game::systems
