#include "engine/engine/core/camera.hpp"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace game {

void clampViewAngles(float& yaw, float& pitch) {
    while (yaw > kPi) {
        yaw -= 2.0f * kPi;
    }
    while (yaw < -kPi) {
        yaw += 2.0f * kPi;
    }
    pitch = std::clamp(pitch, -kPitchLimit, kPitchLimit);
}

void CCamera::setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane) {
    m_projection = glm::perspectiveRH_ZO(glm::radians(fovDegrees), aspect, nearPlane, farPlane);
}

void CCamera::setPosition(const glm::vec3& position) {
    m_position = position;
    rebuildView();
}

void CCamera::setAngles(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = pitch;
    clampViewAngles(m_yaw, m_pitch);
    rebuildView();
}

void CCamera::addAngles(float deltaYaw, float deltaPitch) {
    setAngles(m_yaw + deltaYaw, m_pitch + deltaPitch);
}

const glm::vec3& CCamera::position() const {
    return m_position;
}

float CCamera::yaw() const {
    return m_yaw;
}

float CCamera::pitch() const {
    return m_pitch;
}

glm::vec3 CCamera::forward() const {
    // yaw=0 iken -Z, pitch yukarı pozitif. Simülasyondaki yön ile aynı.
    return {std::cos(m_pitch) * std::sin(m_yaw), std::sin(m_pitch), -std::cos(m_pitch) * std::cos(m_yaw)};
}

glm::vec3 CCamera::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3{0.0f, 1.0f, 0.0f}));
}

const glm::mat4& CCamera::view() const {
    return m_view;
}

const glm::mat4& CCamera::projection() const {
    return m_projection;
}

glm::mat4 CCamera::viewProjection() const {
    return m_projection * m_view;
}

void CCamera::rebuildView() {
    m_view = glm::lookAtRH(m_position, m_position + forward(), glm::vec3{0.0f, 1.0f, 0.0f});
}

}  // namespace game
