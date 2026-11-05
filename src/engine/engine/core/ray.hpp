#pragma once

// Ray math (header-only, engine-wide). Use IPhysics::raycast for world geometry;
// this is for shapes the physics engine does not see (e.g. the character capsule).

#include <algorithm>
#include <cmath>
#include <optional>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace game {

// Intersection of a ray (origin + t * direction, direction normalized) with a sphere. 0 if the origin is inside.
[[nodiscard]] inline std::optional<float> raySphere(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& center,
                                                    float radius, float maxDistance) {
    const glm::vec3 toOrigin = origin - center;
    const float b = glm::dot(toOrigin, direction);
    const float c = glm::dot(toOrigin, toOrigin) - radius * radius;
    if (c <= 0.0f) {
        return 0.0f;
    }
    if (b > 0.0f) {
        return std::nullopt;  // we are outside and moving away from the sphere
    }
    const float discriminant = b * b - c;
    if (discriminant < 0.0f) {
        return std::nullopt;
    }
    const float t = -b - std::sqrt(discriminant);
    return t <= maxDistance ? std::optional<float>{t} : std::nullopt;
}

// Intersection of a ray with a vertical capsule. The capsule starts at the feet point: total height `height`, radius `radius`.
// Returns: the distance to the nearest hit along the ray (0 if the origin is inside).
[[nodiscard]] inline std::optional<float> rayCapsule(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                                                     const glm::vec3& feet, float radius, float height) {
    const glm::vec3 bottom = feet + glm::vec3{0.0f, radius, 0.0f};          // bottom sphere center
    const glm::vec3 top = feet + glm::vec3{0.0f, height - radius, 0.0f};  // top sphere center

    std::optional<float> best;
    const auto consider = [&best](std::optional<float> t) {
        if (t && (!best || *t < *best)) {
            best = t;
        }
    };

    consider(raySphere(origin, direction, bottom, radius, maxDistance));
    consider(raySphere(origin, direction, top, radius, maxDistance));

    // Cylinder body: circle in the xz plane, y range [bottom.y, top.y].
    const float ox = origin.x - feet.x;
    const float oz = origin.z - feet.z;
    const float a = direction.x * direction.x + direction.z * direction.z;
    const float c = ox * ox + oz * oz - radius * radius;
    if (a > 1e-8f) {
        const float b = ox * direction.x + oz * direction.z;
        const float discriminant = b * b - a * c;
        if (discriminant >= 0.0f) {
            const float t = std::max(0.0f, (-b - std::sqrt(discriminant)) / a);
            const float y = origin.y + t * direction.y;
            if (t <= maxDistance && y >= bottom.y && y <= top.y) {
                consider(t);
            }
        }
    } else if (c <= 0.0f && origin.y >= bottom.y && origin.y <= top.y) {
        consider(0.0f);  // we are looking along the axis and already inside
    }
    return best;
}

}  // namespace game
