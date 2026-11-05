#include "engine/render/shaders/shader_sources.hpp"

namespace game {

// MSL kaynağı runtime'da derlenir (SDL_GPU Metal backend). Uniform struct'ları render/gpu_structs.hpp ile aynı düzende olmalı.
// Kaynak bağlama: vertex buffer(0)=PassUniforms, buffer(1)=DrawUniforms; fragment buffer(0)=FrameUniforms, buffer(1)=MaterialUniforms;
// fragment texture(0)=gölge haritası (depth), texture(1)=baseColor; sampler(0)/(1) aynı sırada.
std::string_view mslShaderSource() {
    static constexpr std::string_view kSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct PassUniforms_t {
    float4x4 viewProjection;
    float4x4 lightViewProjection;
};

struct DrawUniforms_t {
    float4x4 model;
};

struct Light_t {
    float4 positionRange;
    float4 directionType;   // xyz: ışığın gittiği yön, w: 0 yön / 1 nokta / 2 spot
    float4 colorIntensity;
    float4 cone;            // x: cos(inner), y: cos(outer)
};

struct FrameUniforms_t {
    float4x4 invViewProjection;
    float4 cameraPosition;
    float4 sunDirection;    // xyz: yüzeyden güneşe (normalize)
    float4 sunColor;        // rgb: renk * şiddet
    float4 skyColor;
    float4 horizonColor;
    float4 groundColor;
    float4 params0;         // exposure, bloomStrength, fogDensity, shadowTexel
    float4 params1;         // hdrWidth, hdrHeight, time, swapchainIsSrgb
    float4 params2;         // lightCount, ambientIntensity, bloomThreshold, vignette
    float4 params3;         // shadowsEnabled, tonemapper, 0, 0
    Light_t lights[16];
};

struct MaterialUniforms_t {
    float4 baseColor;
    float4 emissiveGrid;    // rgb: emissive, a: gridStrength
    float4 params;          // x: specularPower, y: specularAmount, z: rimAmount
};

struct VertexIn_t {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
};

// kPosNormalUvSkin: VertexIn_t + joint indeksleri (ubyte4 normalized: indeks = round(x * 255)) + ağırlıklar.
struct SkinnedVertexIn_t {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
    float4 joints   [[attribute(3)]];
    float4 weights  [[attribute(4)]];
};

struct BoneUniforms_t {
    float4x4 bones[128];    // kMaxBones: düğüm dünya * inverseBind
};

// Vertex'i modelin uzayında skin matrislerinin ağırlıklı toplamıyla dönüştürür.
float4x4 skinMatrix(SkinnedVertexIn_t in, constant BoneUniforms_t& b) {
    uint4 j = uint4(in.joints * 255.0 + 0.5);
    return b.bones[j.x] * in.weights.x + b.bones[j.y] * in.weights.y +
           b.bones[j.z] * in.weights.z + b.bones[j.w] * in.weights.w;
}

// ---------------------------------------------------------------- shadow
struct ShadowOut_t {
    float4 position [[position]];
};

vertex ShadowOut_t vertex_shadow(VertexIn_t in [[stage_in]],
                                 constant PassUniforms_t& passData [[buffer(0)]],
                                 constant DrawUniforms_t& draw [[buffer(1)]]) {
    ShadowOut_t out;
    out.position = passData.viewProjection * (draw.model * float4(in.position, 1.0));
    return out;
}

vertex ShadowOut_t vertex_shadow_skinned(SkinnedVertexIn_t in [[stage_in]],
                                         constant PassUniforms_t& passData [[buffer(0)]],
                                         constant DrawUniforms_t& draw [[buffer(1)]],
                                         constant BoneUniforms_t& bones [[buffer(2)]]) {
    ShadowOut_t out;
    out.position = passData.viewProjection * (draw.model * (skinMatrix(in, bones) * float4(in.position, 1.0)));
    return out;
}

fragment void fragment_shadow() {}

// ---------------------------------------------------------------- scene
struct SceneOut_t {
    float4 position [[position]];
    float3 worldPosition;
    float3 normal;
    float3 shadowPosition;
    float2 uv;
};

// position/normal modelin (varsa skin uygulanmış) uzayındadır.
SceneOut_t makeSceneOut(float3 position, float3 normal, float2 uv, constant PassUniforms_t& passData, constant DrawUniforms_t& draw) {
    SceneOut_t out;
    float4 world = draw.model * float4(position, 1.0);
    // Normal: non-uniform scale için 3x3 cofactor yaklaşımı.
    float3x3 m3 = float3x3(draw.model[0].xyz, draw.model[1].xyz, draw.model[2].xyz);
    float3x3 cof = float3x3(cross(m3[1], m3[2]), cross(m3[2], m3[0]), cross(m3[0], m3[1]));
    float3 n = normalize(cof * normal);
    out.position = passData.viewProjection * world;
    out.worldPosition = world.xyz;
    out.normal = n;
    // Normal-offset: gölge acne'sini azaltır.
    out.shadowPosition = (passData.lightViewProjection * float4(world.xyz + n * 0.05, 1.0)).xyz;
    out.uv = uv;
    return out;
}

vertex SceneOut_t vertex_scene(VertexIn_t in [[stage_in]],
                               constant PassUniforms_t& passData [[buffer(0)]],
                               constant DrawUniforms_t& draw [[buffer(1)]]) {
    return makeSceneOut(in.position, in.normal, in.uv, passData, draw);
}

vertex SceneOut_t vertex_scene_skinned(SkinnedVertexIn_t in [[stage_in]],
                                       constant PassUniforms_t& passData [[buffer(0)]],
                                       constant DrawUniforms_t& draw [[buffer(1)]],
                                       constant BoneUniforms_t& bones [[buffer(2)]]) {
    float4x4 skin = skinMatrix(in, bones);
    // Normal: skin matrisinin dönme kısmı (ölçeksiz iskelet varsayımı).
    float3 skinnedNormal = (skin * float4(in.normal, 0.0)).xyz;
    return makeSceneOut((skin * float4(in.position, 1.0)).xyz, skinnedNormal, in.uv, passData, draw);
}

float shadowFactor(depth2d<float> shadowMap, sampler s, float3 sc, float ndl, float texel) {
    float2 uv = float2(sc.x * 0.5 + 0.5, 0.5 - sc.y * 0.5);
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || sc.z > 1.0) {
        return 1.0;
    }
    float bias = 0.00008 + 0.00025 * (1.0 - ndl);
    float lit = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float stored = shadowMap.sample(s, uv + float2(x, y) * texel);
            lit += (sc.z - bias > stored) ? 0.0 : 1.0;
        }
    }
    return lit / 9.0;
}

float3 evalLight(Light_t l, float3 worldPosition, float3 n, float3 V, float3 albedo, float specPower, float specAmount) {
    float type = l.directionType.w;
    float3 L;
    float atten = 1.0;
    if (type < 0.5) {
        L = -normalize(l.directionType.xyz);
    } else {
        float3 toLight = l.positionRange.xyz - worldPosition;
        float dist = max(length(toLight), 1e-4);
        L = toLight / dist;
        float falloff = saturate(1.0 - pow(dist / max(l.positionRange.w, 1e-4), 4.0));
        atten = falloff * falloff / (dist * dist + 1.0);
        if (type > 1.5) {
            float cd = dot(-L, normalize(l.directionType.xyz));
            atten *= smoothstep(l.cone.y, l.cone.x, cd);
        }
    }
    float ndl = max(dot(n, L), 0.0);
    float3 H = normalize(L + V);
    float spec = pow(max(dot(n, H), 0.0), specPower) * specAmount * step(0.0001, ndl);
    return (albedo * ndl + spec) * l.colorIntensity.rgb * atten;
}

float3 shadeSurface(SceneOut_t in, constant FrameUniforms_t& f, depth2d<float> shadowMap, sampler shadowSampler,
                    float3 albedo, float3 n, float specPower, float specAmount) {
    float3 L = normalize(f.sunDirection.xyz);
    float3 V = normalize(f.cameraPosition.xyz - in.worldPosition);
    float ndl = max(dot(n, L), 0.0);
    float shadow = 1.0;
    if (f.params3.x > 0.5) {
        shadow = shadowFactor(shadowMap, shadowSampler, in.shadowPosition, ndl, f.params0.w);
    }

    float3 ambient = mix(f.groundColor.rgb, f.skyColor.rgb, n.y * 0.5 + 0.5) * f.params2.y;
    float3 H = normalize(L + V);
    float spec = pow(max(dot(n, H), 0.0), specPower) * specAmount * step(0.0001, ndl);
    float3 color = albedo * ambient * 0.9 + (albedo * ndl + spec) * f.sunColor.rgb * shadow;

    int count = min(int(f.params2.x), 16);
    for (int i = 0; i < count; ++i) {
        color += evalLight(f.lights[i], in.worldPosition, n, V, albedo, specPower, specAmount);
    }
    return color;
}

float3 applyFog(float3 color, constant FrameUniforms_t& f, float3 worldPosition) {
    float dist = length(f.cameraPosition.xyz - worldPosition);
    return mix(f.horizonColor.rgb, color, exp(-dist * f.params0.z));
}

struct Surface_t {
    float3 albedo;
    float alpha;
    float3 normal;
};

Surface_t readSurface(SceneOut_t in, constant MaterialUniforms_t& m, texture2d<float> baseTexture, sampler baseSampler) {
    float4 texel = baseTexture.sample(baseSampler, in.uv);
    Surface_t s;
    s.albedo = pow(m.baseColor.rgb, float3(2.2)) * texel.rgb;  // sanatsal renk (sRGB) -> lineer
    s.alpha = m.baseColor.a * texel.a;
    s.normal = normalize(in.normal);

    // Kareli desen + ince çizgiler (hareketi görmek için), dünya XZ düzleminde.
    float strength = m.emissiveGrid.w;
    if (strength > 0.0) {
        float2 p = in.worldPosition.xz;
        float2 w = fwidth(p);
        float2 edge = abs(fract(p - 0.5) - 0.5) / max(w, float2(1e-4));
        float lineMask = 1.0 - min(min(edge.x, edge.y), 1.0);
        lineMask *= 1.0 - smoothstep(0.15, 0.5, max(w.x, w.y));
        float checker = fmod(floor(p.x) + floor(p.y), 2.0);
        s.albedo *= mix(1.0, mix(0.78, 1.0, abs(checker)), strength);
        s.albedo = mix(s.albedo, s.albedo * 0.45, lineMask * 0.6 * strength);
    }
    return s;
}

fragment float4 fragment_lit(SceneOut_t in [[stage_in]],
                             constant FrameUniforms_t& f [[buffer(0)]],
                             constant MaterialUniforms_t& m [[buffer(1)]],
                             depth2d<float> shadowMap [[texture(0)]],
                             texture2d<float> baseTexture [[texture(1)]],
                             sampler shadowSampler [[sampler(0)]],
                             sampler baseSampler [[sampler(1)]]) {
    Surface_t s = readSurface(in, m, baseTexture, baseSampler);
    float3 color = shadeSurface(in, f, shadowMap, shadowSampler, s.albedo, s.normal, m.params.x, m.params.y);
    color += m.emissiveGrid.rgb;
    return float4(applyFog(color, f, in.worldPosition), s.alpha);
}

// Gövde gradyanı (ayak koyu, baş açık) + fresnel rim ışığı.
fragment float4 fragment_rimlit(SceneOut_t in [[stage_in]],
                                constant FrameUniforms_t& f [[buffer(0)]],
                                constant MaterialUniforms_t& m [[buffer(1)]],
                                depth2d<float> shadowMap [[texture(0)]],
                                texture2d<float> baseTexture [[texture(1)]],
                                sampler shadowSampler [[sampler(0)]],
                                sampler baseSampler [[sampler(1)]]) {
    Surface_t s = readSurface(in, m, baseTexture, baseSampler);
    s.albedo *= mix(0.6, 1.1, saturate(in.worldPosition.y / 1.8));
    float3 color = shadeSurface(in, f, shadowMap, shadowSampler, s.albedo, s.normal, m.params.x, m.params.y);

    float3 V = normalize(f.cameraPosition.xyz - in.worldPosition);
    float rim = pow(1.0 - saturate(dot(s.normal, V)), 3.0);
    color += mix(f.horizonColor.rgb, s.albedo, 0.5) * rim * m.params.z;
    color += m.emissiveGrid.rgb;
    return float4(applyFog(color, f, in.worldPosition), s.alpha);
}

fragment float4 fragment_unlit(SceneOut_t in [[stage_in]],
                               constant FrameUniforms_t& f [[buffer(0)]],
                               constant MaterialUniforms_t& m [[buffer(1)]],
                               depth2d<float> shadowMap [[texture(0)]],
                               texture2d<float> baseTexture [[texture(1)]],
                               sampler shadowSampler [[sampler(0)]],
                               sampler baseSampler [[sampler(1)]]) {
    Surface_t s = readSurface(in, m, baseTexture, baseSampler);
    float3 color = s.albedo + m.emissiveGrid.rgb;
    return float4(applyFog(color, f, in.worldPosition), s.alpha);
}

// ---------------------------------------------------------------- fullscreen
struct FullscreenOut_t {
    float4 position [[position]];
    float2 uv;
};

vertex FullscreenOut_t vertex_fullscreen(uint vid [[vertex_id]]) {
    float2 p = float2(float((vid << 1) & 2u), float(vid & 2u));
    FullscreenOut_t out;
    out.position = float4(p * 2.0 - 1.0, 0.0, 1.0);
    out.uv = float2(p.x, 1.0 - p.y);
    return out;
}

fragment float4 fragment_sky(FullscreenOut_t in [[stage_in]], constant FrameUniforms_t& f [[buffer(0)]]) {
    float2 ndc = float2(in.uv.x * 2.0 - 1.0, 1.0 - in.uv.y * 2.0);
    float4 farP = f.invViewProjection * float4(ndc, 1.0, 1.0);
    float4 nearP = f.invViewProjection * float4(ndc, 0.0, 1.0);
    float3 dir = normalize(farP.xyz / farP.w - nearP.xyz / nearP.w);

    float up = dir.y;
    float3 sky = mix(f.horizonColor.rgb, f.skyColor.rgb, pow(saturate(up), 0.55));
    float3 below = mix(f.horizonColor.rgb, f.groundColor.rgb * 3.0, saturate(-up * 4.0));
    float3 color = up >= 0.0 ? sky : below;

    float3 L = normalize(f.sunDirection.xyz);
    float s = max(dot(dir, L), 0.0);
    color += f.sunColor.rgb * (pow(s, 24.0) * 0.12 + pow(s, 400.0) * 0.6);
    color += f.sunColor.rgb * smoothstep(0.9996, 0.9999, s) * 4.0;  // güneş diski (bloom'a girer)
    return float4(color, 1.0);
}

// ---------------------------------------------------------------- post
float3 acesFilm(float3 x) {
    return saturate((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14));
}

float3 tonemap(float3 c, float mode) {
    if (mode < 0.5) {
        return acesFilm(c);
    }
    if (mode < 1.5) {
        return c / (1.0 + c);
    }
    return saturate(c);
}

float3 linearToSrgb(float3 c) {
    return select(1.055 * pow(c, float3(1.0 / 2.4)) - 0.055, 12.92 * c, c <= 0.0031308);
}

fragment float4 fragment_post(FullscreenOut_t in [[stage_in]],
                              constant FrameUniforms_t& f [[buffer(0)]],
                              texture2d<float> hdr [[texture(0)]],
                              sampler s [[sampler(0)]]) {
    float2 px = 1.0 / f.params1.xy;
    float3 color = hdr.sample(s, in.uv, level(0)).rgb;

    // Bloom: eşiğin üstündeki parlak kısımların geniş gauss bulanıklığı.
    if (f.params0.y > 0.0) {
        float3 bloom = float3(0.0);
        float weightSum = 0.0;
        for (int i = -4; i <= 4; ++i) {
            for (int j = -4; j <= 4; ++j) {
                float w = exp(-float(i * i + j * j) / 12.0);
                float3 c = hdr.sample(s, in.uv + float2(i, j) * px * 3.0, level(0)).rgb;
                float lum = max(c.r, max(c.g, c.b));
                bloom += c * (max(lum - f.params2.z, 0.0) / max(lum, 1e-4)) * w;
                weightSum += w;
            }
        }
        color += bloom / weightSum * f.params0.y * 4.0;
    }

    color = tonemap(color * f.params0.x, f.params3.y);

    float d = length(in.uv - 0.5);
    color *= 1.0 - f.params2.w * smoothstep(0.3, 0.85, d);

    if (f.params1.w < 0.5) {
        color = linearToSrgb(color);
    }
    return float4(color, 1.0);
}

// ---------------------------------------------------------------- debug çizgileri
struct DebugIn_t {
    float3 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
};

struct DebugOut_t {
    float4 position [[position]];
    float4 color;
};

vertex DebugOut_t vertex_debug(DebugIn_t in [[stage_in]], constant PassUniforms_t& passData [[buffer(0)]]) {
    DebugOut_t out;
    out.position = passData.viewProjection * float4(in.position, 1.0);
    out.color = in.color;
    return out;
}

fragment float4 fragment_debug(DebugOut_t in [[stage_in]]) {
    return in.color;
}
)MSL";
    return kSource;
}

}  // namespace game
