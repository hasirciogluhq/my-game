#pragma once

#include "engine/interface/i_platform.hpp"
#include "game/client/input/input_map.hpp"
#include "game/shared/game_types.hpp"

#include <cstdint>

namespace game {

// Produces one UserCmd per client tick. Keys are read from the action table (Bindings_t)
// and converted to game actions (EButton). Held state is `buttons`; changes are `buttonsPressed/Released`.
class CInputSampler {
public:
    void bind(const IPlatform* platform, const Bindings_t* bindings);
    void addMouseDelta(float dx, float dy);
    [[nodiscard]] UserCmd_t sample(uint32_t tick);

    void setMouseSensitivity(float radiansPerPixel);
    // While disabled (console open), keys and mouse are not read; the command only carries the current view angles.
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
