#pragma once

#include "engine/interface/i_platform.hpp"
#include "game/client/input/input_map.hpp"
#include "game/shared/game_types.hpp"

#include <cstdint>

namespace game {

// Her client tick'inde bir UserCmd üretir. Tuşlar aksiyon tablosundan (Bindings_t) okunur,
// oyun aksiyonlarına (EButton) çevrilir. Basılı durum `buttons`, değişim `buttonsPressed/Released`.
class CInputSampler {
public:
    void bind(const IPlatform* platform, const Bindings_t* bindings);
    void addMouseDelta(float dx, float dy);
    [[nodiscard]] UserCmd_t sample(uint32_t tick);

    void setMouseSensitivity(float radiansPerPixel);
    // Kapalıyken (konsol açık) tuş ve fare okunmaz; komut sadece mevcut bakış açısını taşır.
    void setEnabled(bool enabled);

private:
    const IPlatform* m_platform = nullptr;
    const Bindings_t* m_bindings = nullptr;
    bool m_enabled = true;
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_pendingDx = 0.0f;
    float m_pendingDy = 0.0f;
    float m_mouseSensitivity = 0.0025f;
    uint32_t m_previousButtons = 0;
};

}  // namespace game
