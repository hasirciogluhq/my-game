#pragma once

// Işın matematiği (header-only, motor genel). Dünya geometrisi için IPhysics::raycast kullanılır;
// burası fizik motorunun görmediği şekiller içindir (ör. karakter kapsülü).

#include <algorithm>
#include <cmath>
#include <optional>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace game {

// Işın (origin + t * direction, direction normalize) ile küre kesişimi. Başlangıç içerideyse 0.
[[nodiscard]] inline std::optional<float> raySphere(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& center,
                                                    float radius, float maxDistance) {
    const glm::vec3 toOrigin = origin - center;
    const float b = glm::dot(toOrigin, direction);
    const float c = glm::dot(toOrigin, toOrigin) - radius * radius;
    if (c <= 0.0f) {
        return 0.0f;
    }
    if (b > 0.0f) {
        return std::nullopt;  // dışarıdayız ve küreden uzaklaşıyoruz
    }
    const float discriminant = b * b - c;
    if (discriminant < 0.0f) {
        return std::nullopt;
    }
    const float t = -b - std::sqrt(discriminant);
    return t <= maxDistance ? std::optional<float>{t} : std::nullopt;
}

// Işın ile dikey kapsül kesişimi. Kapsül ayak noktasından başlar: toplam boy `height`, yarıçap `radius`.
// Dönen değer: ışın boyunca en yakın isabet mesafesi (başlangıç içerideyse 0).
[[nodiscard]] inline std::optional<float> rayCapsule(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                                                     const glm::vec3& feet, float radius, float height) {
    const glm::vec3 bottom = feet + glm::vec3{0.0f, radius, 0.0f};          // alt küre merkezi
    const glm::vec3 top = feet + glm::vec3{0.0f, height - radius, 0.0f};  // üst küre merkezi

    std::optional<float> best;
    const auto consider = [&best](std::optional<float> t) {
        if (t && (!best || *t < *best)) {
            best = t;
        }
    };

    consider(raySphere(origin, direction, bottom, radius, maxDistance));
    consider(raySphere(origin, direction, top, radius, maxDistance));

    // Silindir gövdesi: xz düzleminde daire, y aralığı [bottom.y, top.y].
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
        consider(0.0f);  // eksen boyunca bakıyoruz ve zaten içindeyiz
    }
    return best;
}

}  // namespace game
