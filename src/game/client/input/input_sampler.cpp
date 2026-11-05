#include "game/client/input/input_sampler.hpp"

#include "engine/engine/core/camera.hpp"

#include <array>

namespace game {
namespace {

// Action -> network button. For a new button, just add a row to this table.
struct ButtonAction_t {
    EButton button;
    EAction action;
};

constexpr std::array kButtonActions{
    ButtonAction_t{EButton::kAttack, EAction::kAttack},
    ButtonAction_t{EButton::kJump, EAction::kJump},
    ButtonAction_t{EButton::kCrouch, EAction::kCrouch},
    ButtonAction_t{EButton::kUse, EAction::kUse},
    ButtonAction_t{EButton::kReload, EAction::kReload},
    ButtonAction_t{EButton::kWalk, EAction::kWalk},
};

}  // namespace

void CInputSampler::bind(const IPlatform* platform, const Bindings_t* bindings) {
    m_platform = platform;
    m_bindings = bindings;
}

void CInputSampler::addMouseDelta(float dx, float dy) {
    if (m_enabled) {
        m_pendingDx += dx;
        m_pendingDy += dy;
    }
}

void CInputSampler::setMouseSensitivity(float radiansPerPixel) {
    m_mouseSensitivity = radiansPerPixel;
}

void CInputSampler::setEnabled(bool enabled) {
    m_enabled = enabled;
    m_pendingDx = 0.0f;
    m_pendingDy = 0.0f;
}

UserCmd_t CInputSampler::sample(uint32_t tick) {
    // Turning the view right increases yaw (forward.x = sin(yaw)); mouse right (dx > 0) = look right.
    m_yaw += m_pendingDx * m_mouseSensitivity;
    m_pitch -= m_pendingDy * m_mouseSensitivity;
    clampViewAngles(m_yaw, m_pitch);
    m_pendingDx = 0.0f;
    m_pendingDy = 0.0f;

    UserCmd_t cmd;
    cmd.tick = tick;
    cmd.yaw = m_yaw;
    cmd.pitch = m_pitch;

    uint32_t held = 0;
    if (m_platform != nullptr && m_bindings != nullptr && m_enabled) {
        const auto down = [&](EAction action) { return input::isDown(*m_platform, *m_bindings, action); };
        cmd.forwardMove = (down(EAction::kForward) ? 1.0f : 0.0f) - (down(EAction::kBack) ? 1.0f : 0.0f);
        cmd.sideMove = (down(EAction::kRight) ? 1.0f : 0.0f) - (down(EAction::kLeft) ? 1.0f : 0.0f);
        for (const auto& entry : kButtonActions) {
            if (down(entry.action)) {
                held |= buttonBit(entry.button);
            }
        }
    }

    cmd.buttons = held;
    cmd.buttonsPressed = held & ~m_previousButtons;
    cmd.buttonsReleased = m_previousButtons & ~held;
    m_previousButtons = held;
    return cmd;
}

}  // namespace game
