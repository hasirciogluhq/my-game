#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace game {

// physics.dll arayüzü (Jolt). Statik dünya + karakter kapsülleri + ışın testi.
// CharacterHandle_t 0 geçersizdir.
using CharacterHandle_t = uint32_t;

// Karakter kapsülü: ayak noktası orijin, toplam boy ayaktan tepeye. Fizik ve ışın testi aynı değerleri kullanır.
inline constexpr float kCharacterRadius = 0.3f;
inline constexpr float kCharacterHeight = 1.8f;

struct RayHit_t {
    bool hit = false;
    float distance = 0.0f;
    glm::vec3 point{0.0f};
};

struct CharacterResult_t {
    glm::vec3 position{0.0f};  // ayak konumu
    bool onGround = false;
};

class IPhysics {
public:
    virtual ~IPhysics() = default;
    [[nodiscard]] virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void addStaticBox(const glm::vec3& center, const glm::vec3& halfExtents) = 0;

    [[nodiscard]] virtual CharacterHandle_t createCharacter(const glm::vec3& feetPosition) = 0;
    virtual void destroyCharacter(CharacterHandle_t handle) = 0;
    // Karakteri velocity * dt kadar hareket ettirir; duvar/zemin/kutulara çarpar ve kayar.
    [[nodiscard]] virtual CharacterResult_t moveCharacter(CharacterHandle_t handle, const glm::vec3& velocity, float dt) = 0;

    [[nodiscard]] virtual RayHit_t raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const = 0;

    // Açıkken karakterler birbirinin içinden geçemez (server ayarı player.collisions.enabled). Varsayılan: kapalı.
    virtual void setCharacterCollisions(bool enabled) = 0;
    // Karakteri ayak konumuna ışınlar (hız sıfırlanır). Client prediction'ın reconcile adımı kullanır.
    virtual void setCharacterPosition(CharacterHandle_t handle, const glm::vec3& feetPosition) = 0;
};

inline constexpr char kPhysicsInterface[] = "Physics001";

}  // namespace game
