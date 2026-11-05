#pragma once

#include "engine/engine/api.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace game {

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kPitchLimit = 89.0f * kPi / 180.0f;  // ±89 degrees

// Clamps view angles: yaw [-pi, pi], pitch [-kPitchLimit, kPitchLimit].
// The server and the client use the same function.
ENGINE_API void clampViewAngles(float& yaw, float& pitch);

// Builds view/projection from position + view angles. Depth 0..1 for Metal (RH_ZO).
class ENGINE_API CCamera {
public:
    void setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane);

    void setPosition(const glm::vec3& position);
    void setAngles(float yaw, float pitch);   // clamp is applied here
    void addAngles(float deltaYaw, float deltaPitch);

    [[nodiscard]] const glm::vec3& position() const;
    [[nodiscard]] float yaw() const;
    [[nodiscard]] float pitch() const;
    [[nodiscard]] glm::vec3 forward() const;
    [[nodiscard]] glm::vec3 right() const;

    [[nodiscard]] const glm::mat4& view() const;
    [[nodiscard]] const glm::mat4& projection() const;
    [[nodiscard]] glm::mat4 viewProjection() const;

private:
    void rebuildView();

    glm::vec3 m_position{0.0f, 1.6f, 0.0f};
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    glm::mat4 m_view{1.0f};
    glm::mat4 m_projection{1.0f};
};

}  // namespace game
