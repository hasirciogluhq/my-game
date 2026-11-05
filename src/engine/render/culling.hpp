#pragma once

#include <algorithm>
#include <array>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace game::culling {

struct Frustum_t {
    std::array<glm::vec4, 6> planes;  // xyz: normal pointing inward (normalized), w: distance
};

// Gribb-Hartmann, depth 0..1 (ZO). viewProjection = projection * view.
[[nodiscard]] inline Frustum_t makeFrustum(const glm::mat4& vp) {
    const auto row = [&vp](int i) { return glm::vec4{vp[0][i], vp[1][i], vp[2][i], vp[3][i]}; };
    Frustum_t frustum{{row(3) + row(0), row(3) - row(0), row(3) + row(1), row(3) - row(1), row(2), row(3) - row(2)}};
    for (glm::vec4& plane : frustum.planes) {
        const float length = glm::length(glm::vec3{plane});
        if (length > 0.0f) {
            plane /= length;
        }
    }
    return frustum;
}

[[nodiscard]] inline bool intersectsSphere(const Frustum_t& frustum, const glm::vec3& center, float radius) {
    return std::ranges::all_of(frustum.planes, [&](const glm::vec4& plane) {
        return glm::dot(glm::vec3{plane}, center) + plane.w >= -radius;
    });
}

// Largest axis scale of the model matrix (to carry the bounding sphere radius into world space).
[[nodiscard]] inline float maxAxisScale(const glm::mat4& model) {
    return std::max({glm::length(glm::vec3{model[0]}), glm::length(glm::vec3{model[1]}), glm::length(glm::vec3{model[2]})});
}

}  // namespace game::culling
