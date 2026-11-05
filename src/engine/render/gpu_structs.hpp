#pragma once

// CPU <-> shader uniform düzeni. shaders/msl_source.cpp içindeki aynı isimli struct'larla BİREBİR aynı olmalı
// (her şey float4 / float4x4 tabanlı; padding yok). Yeni dil (HLSL) eklenirse cbuffer düzeni de bu olmalı.
// Slot düzeni:
//   vertex   b0 = PassUniforms_t   (pass başına)     vertex   b1 = DrawUniforms_t     (draw başına)
//   fragment b0 = FrameUniforms_t  (frame başına)    fragment b1 = MaterialUniforms_t (material başına)

#include "engine/interface/render_types.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace game::gpu {

inline constexpr uint32_t kPassUniformSlot = 0;
inline constexpr uint32_t kDrawUniformSlot = 1;
inline constexpr uint32_t kFrameUniformSlot = 0;
inline constexpr uint32_t kMaterialUniformSlot = 1;

struct PassUniforms_t {
    glm::mat4 viewProjection;
    glm::mat4 lightViewProjection;
};

struct DrawUniforms_t {
    glm::mat4 model;
};

struct Light_t {
    glm::vec4 positionRange;    // xyz: konum, w: menzil
    glm::vec4 directionType;    // xyz: ışığın gittiği yön (normalize), w: 0 yön / 1 nokta / 2 spot
    glm::vec4 colorIntensity;   // rgb: renk * şiddet
    glm::vec4 cone;             // x: cos(inner), y: cos(outer)
};

struct FrameUniforms_t {
    glm::mat4 invViewProjection;
    glm::vec4 cameraPosition;
    glm::vec4 sunDirection;     // xyz: yüzeyden güneşe (normalize)
    glm::vec4 sunColor;         // rgb: renk * şiddet
    glm::vec4 skyColor;
    glm::vec4 horizonColor;
    glm::vec4 groundColor;
    glm::vec4 params0;          // exposure, bloomStrength, fogDensity, shadowTexel
    glm::vec4 params1;          // hdrWidth, hdrHeight, time, swapchainIsSrgb
    glm::vec4 params2;          // lightCount, ambientIntensity, bloomThreshold, vignette
    glm::vec4 params3;          // shadowsEnabled, tonemapper, 0, 0
    Light_t lights[kMaxLights];
};

struct MaterialUniforms_t {
    glm::vec4 baseColor;        // sRGB, a = opaklık
    glm::vec4 emissiveGrid;     // rgb: emissive, a: gridStrength
    glm::vec4 params;           // x: specularPower, y: specularAmount, z: rimAmount, w: 0
};

struct DebugVertex_t {
    glm::vec3 position;
    glm::vec4 color;
};

static_assert(sizeof(PassUniforms_t) == 128);
static_assert(sizeof(DrawUniforms_t) == 64);
static_assert(sizeof(Light_t) == 64);
static_assert(sizeof(FrameUniforms_t) == 64 + 10 * 16 + kMaxLights * 64);
static_assert(sizeof(MaterialUniforms_t) == 48);
static_assert(sizeof(DebugVertex_t) == 28);

}  // namespace game::gpu
