#pragma once

#include "game/shared/snapshot.hpp"

#include <deque>

// Snapshot interpolation (uzaktaki oyuncular + mermiler). Oyun mantığı: engine/ değil game/.
// Client, gösterimi server zamanının biraz gerisinde (delaySeconds) tutar ve iki snapshot arasını karıştırır.
// Extrapolation yok: gösterim zamanı en yeni snapshot'ı geçmez.
namespace game::interpolation {

inline constexpr size_t kMaxSnapshots = 32;
inline constexpr uint32_t kMinDelayTicks = 2;
inline constexpr double kCatchUp = 0.1;       // gösterim zamanı hedefe tick başına bu oranda yaklaşır
inline constexpr double kJumpThreshold = 3.0; // hedeften bu kadar tick uzaksa doğrudan hedefe sıçra

struct Interpolation_t {
    std::deque<WorldSnapshot_t> snapshots;  // tick sırasıyla
    double renderTick = 0.0;                // server tick cinsinden gösterim zamanı
    float delaySeconds = 0.05f;             // cl_interp
    bool enabled = true;                    // cl_interpolate
};

// Yeni snapshot ekler (tick'i son eklenenden büyük olmalı).
void push(Interpolation_t& state, WorldSnapshot_t snapshot);

// Her client tick: gösterim zamanını ilerletir, kullanılmayan eski snapshot'ları atar.
void advance(Interpolation_t& state, uint32_t tickRate);

// Server zamanında ekranda gösterilen an: interpolation açıkken renderTick, henüz snapshot yoksa world.tick.
// Mermi konumu ve animasyon pozu gibi "zamandan hesaplanan" şeyler bunu kullanır.
[[nodiscard]] double displayTick(const Interpolation_t& state, uint32_t worldTick);

// Yerel oyuncu hariç tüm oyuncuların Transform_t'sini renderTick'teki karışık değere yazar.
void apply(World_t& world, const Interpolation_t& state, uint32_t localPlayerId);

// Açıları en kısa yoldan karıştırır (sarma: pi -> -pi ters yöne dönmez).
[[nodiscard]] float lerpAngle(float from, float to, float t);

}  // namespace game::interpolation
