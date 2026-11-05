#pragma once

#include "game/shared/world.hpp"

#include <glm/vec3.hpp>

namespace game::death_cam {

// Pawn'ı olmayan oyuncunun kamerası: haritanın üstünden, karakterlere bakarak yavaşça dönen havadan bakış
// (Battlefield / GTA V yükleme ekranı gibi).
struct DeathCam_t {
    float angle = 0.0f;  // dönüş açısı (rad)
};

struct Pose_t {
    glm::vec3 position{0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
};

// Açıyı ilerletir.
void update(DeathCam_t& cam, float dt);

// Odak noktası: hayatta olan karakterlerin ortası (kimse yoksa harita merkezi). Kamera onun etrafında döner.
[[nodiscard]] Pose_t pose(const DeathCam_t& cam, const World_t& world);

}  // namespace game::death_cam
