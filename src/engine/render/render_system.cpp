#include "engine/render/render_system.hpp"

#include "engine/render/culling.hpp"
#include "engine/render/passes/debug_pass.hpp"
#include "engine/render/passes/post_pass.hpp"
#include "engine/render/passes/scene_pass.hpp"
#include "engine/render/passes/shadow_pass.hpp"
#include "engine/render/passes/overlay_pass.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>

namespace game {
namespace {

constexpr float kShadowDepthRange = 120.0f;  // distance of the light camera from its focus (meters)

uint32_t shadowMapSize(EShadowQuality quality) {
    switch (quality) {
        case EShadowQuality::kOff: return 0;
        case EShadowQuality::kLow: return 1024;
        case EShadowQuality::kMedium: return 2048;
        case EShadowQuality::kHigh: return 4096;
    }
    return 0;
}

RenderSettings_t sanitized(RenderSettings_t settings) {
    settings.renderScale = std::clamp(settings.renderScale, 0.25f, 2.0f);
    settings.shadowExtent = std::max(settings.shadowExtent, 1.0f);
    return settings;
}

// Sun ortho matrix covers the area around the camera. The focus is snapped to the texel grid so the shadow does not shimmer when the camera moves.
glm::mat4 computeLightMatrix(const glm::vec3& cameraPosition, const glm::vec3& toSun, float extent, uint32_t shadowSize) {
    const glm::vec3 up = std::abs(toSun.y) > 0.99f ? glm::vec3{0.0f, 0.0f, 1.0f} : glm::vec3{0.0f, 1.0f, 0.0f};
    const glm::vec3 focus{cameraPosition.x, 0.0f, cameraPosition.z};
    const glm::mat4 lightView = glm::lookAtRH(focus + toSun * kShadowDepthRange, focus, up);
    const float texelSize = (2.0f * extent) / static_cast<float>(std::max(shadowSize, 1u));
    const glm::vec3 focusLight{lightView * glm::vec4{focus, 1.0f}};
    const glm::vec3 snap{std::round(focusLight.x / texelSize) * texelSize - focusLight.x,
                         std::round(focusLight.y / texelSize) * texelSize - focusLight.y, 0.0f};
    const glm::mat4 projection = glm::orthoRH_ZO(-extent, extent, -extent, extent, 1.0f, kShadowDepthRange * 2.0f);
    return projection * glm::translate(glm::mat4{1.0f}, snap) * lightView;
}

gpu::Light_t makeGpuLight(const LightDesc_t& light) {
    const glm::vec3 direction = glm::length(light.direction) > 0.0f ? glm::normalize(light.direction) : glm::vec3{0.0f, -1.0f, 0.0f};
    return {.positionRange = glm::vec4{light.position, light.range},
            .directionType = glm::vec4{direction, static_cast<float>(light.type == ELightType::kDirectional ? 0 : light.type == ELightType::kPoint ? 1 : 2)},
            .colorIntensity = glm::vec4{light.color * light.intensity, 1.0f},
            .cone = {std::cos(light.innerConeRadians), std::cos(light.outerConeRadians), 0.0f, 0.0f}};
}

gpu::FrameUniforms_t buildFrameUniforms(const SceneState_t& scene, const rhi::RhiFrameInfo_t& backbuffer, const CRenderTargets& targets,
                                        bool shadowsEnabled) {
    const EnvironmentDesc_t& env = scene.environment;
    const PostProcessDesc_t& post = scene.post;
    const glm::vec3 toSun = glm::length(env.sunDirection) > 0.0f ? -glm::normalize(env.sunDirection) : glm::vec3{0.0f, 1.0f, 0.0f};

    gpu::FrameUniforms_t frame{};
    frame.invViewProjection = glm::inverse(scene.viewProjection);
    frame.cameraPosition = glm::vec4{scene.cameraPosition, 1.0f};
    frame.sunDirection = glm::vec4{toSun, 0.0f};
    frame.sunColor = glm::vec4{env.sunColor * env.sunIntensity, 1.0f};
    frame.skyColor = glm::vec4{env.skyColor, 1.0f};
    frame.horizonColor = glm::vec4{env.horizonColor, 1.0f};
    frame.groundColor = glm::vec4{env.groundColor, 1.0f};
    frame.params0 = {post.exposure, post.bloomStrength, env.fogDensity, 1.0f / static_cast<float>(std::max(targets.shadowSize(), 1u))};
    frame.params1 = {static_cast<float>(targets.width()), static_cast<float>(targets.height()), scene.frame.timeSeconds,
                     rhi::isSrgbFormat(backbuffer.format) ? 1.0f : 0.0f};
    const auto lightCount = static_cast<uint32_t>(std::min<size_t>(scene.lights.size(), kMaxLights));
    frame.params2 = {static_cast<float>(lightCount), env.ambientIntensity, post.bloomThreshold, post.vignette};
    frame.params3 = {shadowsEnabled ? 1.0f : 0.0f, static_cast<float>(post.tonemapper), 0.0f, 0.0f};
    for (uint32_t i = 0; i < lightCount; ++i) {
        frame.lights[i] = makeGpuLight(scene.lights[i]);
    }
    return frame;
}

}  // namespace

CRenderSystem::CRenderSystem() = default;

CRenderSystem::~CRenderSystem() {
    shutdown();
}

bool CRenderSystem::fail(std::string message) {
    m_lastError = std::move(message);
    std::cerr << "render: " << m_lastError << "\n";
    return false;
}

// ---------------------------------------------------------------------------
// lifecycle
// ---------------------------------------------------------------------------

bool CRenderSystem::createPasses() {
    m_passes.clear();
    m_passes.push_back(std::make_unique<CShadowPass>());
    m_passes.push_back(std::make_unique<CScenePass>());
    m_passes.push_back(std::make_unique<CDebugPass>());
    m_passes.push_back(std::make_unique<CPostPass>());
    m_passes.push_back(std::make_unique<COverlayPass>());

    const PassInitContext_t context{.device = *m_device, .shaders = m_shaders, .resources = m_resources};
    for (const auto& pass : m_passes) {
        if (!pass->create(context)) {
            return fail("could not create pass: " + m_device->lastError());
        }
    }
    return true;
}

bool CRenderSystem::initialize(const RendererDesc_t& desc) {
    if (m_initialized) {
        return fail("renderer is already initialized");
    }
    m_settings = sanitized(desc.settings);

    std::string error;
    m_device = rhi::createRhiDevice(desc.backend, error);
    if (m_device == nullptr) {
        return fail(error);
    }
    if (!m_device->initialize({.surface = desc.surface, .vsync = m_settings.vsync, .debugLayer = desc.debugLayer})) {
        const std::string deviceError = m_device->lastError();
        m_device.reset();
        return fail(deviceError);
    }

    m_shaders.bind(m_device.get());
    if (!m_resources.initialize(*m_device) || !createPasses()) {
        const std::string message = m_lastError.empty() ? m_resources.lastError() : m_lastError;
        shutdown();
        return fail(message);
    }

    const rhi::RhiCaps_t& caps = m_device->caps();
    m_caps = {.backend = caps.backend,
              .backendName = caps.backendName,
              .shaderLanguage = caps.shaderLanguage == rhi::EShaderLanguage::kMsl ? "MSL" : caps.shaderLanguage == rhi::EShaderLanguage::kHlsl ? "HLSL" : "SPIR-V",
              .maxTextureSize = caps.maxTextureSize,
              .maxLights = kMaxLights,
              .depthZeroToOne = caps.depthZeroToOne};
    m_initialized = true;
    m_lastError.clear();
    return true;
}

void CRenderSystem::shutdown() {
    if (m_device != nullptr) {
        m_device->waitIdle();
        for (const auto& pass : m_passes) {
            pass->destroy(*m_device);
        }
        m_passes.clear();
        m_targets.release(*m_device);
        m_resources.shutdown();
        m_shaders.destroy();
        m_device->shutdown();
        m_device.reset();
    }
    m_scene.submitted.clear();
    m_scene.boneMatrices.clear();
    m_scene.lights.clear();
    m_scene.debugVertices.clear();
    m_initialized = false;
}

void CRenderSystem::applySettings(const RenderSettings_t& settings) {
    const RenderSettings_t next = sanitized(settings);
    if (m_initialized && next.vsync != m_settings.vsync) {
        m_device->setVsync(next.vsync);
    }
    m_settings = next;  // shadow size and render scale rebuild the targets in the next endFrame
}

// ---------------------------------------------------------------------------
// resources
// ---------------------------------------------------------------------------

MeshHandle_t CRenderSystem::createMesh(const MeshDesc_t& desc) {
    const MeshHandle_t handle = m_initialized ? m_resources.createMesh(desc) : MeshHandle_t{};
    if (!handle.valid()) {
        m_lastError = m_resources.lastError();
    }
    return handle;
}

void CRenderSystem::destroyMesh(MeshHandle_t mesh) {
    if (m_initialized) m_resources.destroyMesh(mesh);
}

TextureHandle_t CRenderSystem::createTexture(const TextureDesc_t& desc) {
    const TextureHandle_t handle = m_initialized ? m_resources.createTexture(desc) : TextureHandle_t{};
    if (!handle.valid()) {
        m_lastError = m_resources.lastError();
    }
    return handle;
}

void CRenderSystem::destroyTexture(TextureHandle_t texture) {
    if (m_initialized) m_resources.destroyTexture(texture);
}

MaterialHandle_t CRenderSystem::createMaterial(const MaterialDesc_t& desc) {
    return m_initialized ? m_resources.createMaterial(desc) : MaterialHandle_t{};
}

void CRenderSystem::updateMaterial(MaterialHandle_t material, const MaterialDesc_t& desc) {
    if (m_initialized) m_resources.updateMaterial(material, desc);
}

void CRenderSystem::destroyMaterial(MaterialHandle_t material) {
    if (m_initialized) m_resources.destroyMaterial(material);
}

// ---------------------------------------------------------------------------
// frame
// ---------------------------------------------------------------------------

void CRenderSystem::beginFrame(const FrameDesc_t& frame) {
    m_scene.frame = frame;
    m_scene.submitted.clear();
    m_scene.boneMatrices.clear();
    m_scene.lights.clear();
    m_scene.debugVertices.clear();
}

void CRenderSystem::submit(const DrawItem_t& item) {
    if (!m_initialized) {
        return;
    }
    SubmittedDraw_t draw{.item = item};
    draw.item.bones = {};  // the span belongs to the caller; the matrices are copied below
    const size_t boneCount = std::min<size_t>(item.bones.size(), kMaxBones);
    draw.boneOffset = static_cast<uint32_t>(m_scene.boneMatrices.size());
    draw.boneCount = static_cast<uint32_t>(boneCount);
    m_scene.boneMatrices.insert(m_scene.boneMatrices.end(), item.bones.begin(), item.bones.begin() + static_cast<std::ptrdiff_t>(boneCount));
    m_scene.submitted.push_back(draw);
}

void CRenderSystem::submitLight(const LightDesc_t& light) {
    if (m_initialized && m_scene.lights.size() < kMaxLights) {
        m_scene.lights.push_back(light);
    }
}

void CRenderSystem::submitDebugLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) {
    if (m_initialized && m_scene.debugVertices.size() < static_cast<size_t>(kMaxDebugLines) * 2) {
        m_scene.debugVertices.push_back({.position = from, .color = color});
        m_scene.debugVertices.push_back({.position = to, .color = color});
    }
}

void CRenderSystem::setOverlay(RenderOverlayFn overlay, void* user) {
    m_scene.overlay = overlay;
    m_scene.overlayUser = user;
}

void* CRenderSystem::nativeDevice() const {
    return m_initialized ? m_device->nativeDevice() : nullptr;
}

// Converts handles to pointers, applies frustum culling, and sorts the opaque/transparent lists.
void CRenderSystem::resolveDraws() {
    SceneState_t& scene = m_scene;
    scene.draws.clear();
    scene.opaque.clear();
    scene.blended.clear();

    const culling::Frustum_t frustum = culling::makeFrustum(scene.viewProjection);
    const MaterialHandle_t fallbackMaterial = m_resources.defaultMaterial();

    for (const SubmittedDraw_t& submitted : scene.submitted) {
        const DrawItem_t& item = submitted.item;
        const GpuMesh_t* mesh = m_resources.find(item.mesh);
        if (mesh == nullptr) {
            continue;
        }
        MaterialHandle_t materialHandle = item.material.valid() ? item.material : fallbackMaterial;
        const MaterialDesc_t* material = m_resources.find(materialHandle);
        if (material == nullptr) {
            materialHandle = fallbackMaterial;
            material = m_resources.find(materialHandle);
        }

        const glm::vec3 localCenter = (mesh->boundsMin + mesh->boundsMax) * 0.5f;
        const bool skinned = mesh->layout == EVertexLayout::kPosNormalUvSkin;
        // Bind-pose bounds can be exceeded during animation (a raised arm): skinned meshes get a radius margin.
        const float localRadius = glm::length(mesh->boundsMax - mesh->boundsMin) * (skinned ? 0.75f : 0.5f);
        const glm::vec3 center{item.transform * glm::vec4{localCenter, 1.0f}};
        const bool visible = culling::intersectsSphere(frustum, center, localRadius * culling::maxAxisScale(item.transform));
        const bool blended = material->blend == EBlendMode::kAlphaBlend;
        const bool castShadow = item.castShadow && material->castShadow && !blended;
        if (!visible) {
            ++m_stats.drawsCulled;
        }
        if (!visible && !castShadow) {
            continue;
        }

        const uint32_t pipelineKey = CScenePass::pipelineKeyFor(*material, skinned);
        const glm::vec3 toCamera = center - scene.cameraPosition;
        scene.draws.push_back({.mesh = mesh,
                               .material = material,
                               .baseTexture = m_resources.baseTextureFor(*material),
                               .model = item.transform,
                               .sortKey = (static_cast<uint64_t>(pipelineKey) << 48) | (static_cast<uint64_t>(materialHandle.index) << 24) | item.mesh.index,
                               .distanceSquared = glm::dot(toCamera, toCamera),
                               .pipelineKey = pipelineKey,
                               .castShadow = castShadow,
                               .skinned = skinned,
                               .boneOffset = submitted.boneOffset,
                               .boneCount = submitted.boneCount});
        if (visible) {
            (blended ? scene.blended : scene.opaque).push_back(static_cast<uint32_t>(scene.draws.size() - 1));
        }
    }

    const auto& draws = scene.draws;
    std::ranges::sort(scene.opaque, {}, [&draws](uint32_t index) { return draws[index].sortKey; });
    std::ranges::sort(scene.blended, std::greater<>{}, [&draws](uint32_t index) { return draws[index].distanceSquared; });
}

void CRenderSystem::prepareScene(uint32_t shadowSize) {
    SceneState_t& scene = m_scene;
    scene.viewProjection = scene.view.projection * scene.view.view;
    scene.cameraPosition = glm::vec3{glm::inverse(scene.view.view)[3]};

    const glm::vec3 sun = scene.environment.sunDirection;
    const glm::vec3 toSun = glm::length(sun) > 0.0f ? -glm::normalize(sun) : glm::vec3{0.0f, 1.0f, 0.0f};
    scene.lightViewProjection = computeLightMatrix(scene.cameraPosition, toSun, m_settings.shadowExtent, shadowSize);
    resolveDraws();
}

void CRenderSystem::renderFrame(const rhi::RhiFrameInfo_t& backbuffer) {
    const uint32_t width = std::max(1u, static_cast<uint32_t>(static_cast<float>(backbuffer.width) * m_settings.renderScale));
    const uint32_t height = std::max(1u, static_cast<uint32_t>(static_cast<float>(backbuffer.height) * m_settings.renderScale));
    const uint32_t shadowSize = shadowMapSize(m_settings.shadowQuality);
    if (!m_targets.ensure(*m_device, width, height, shadowSize)) {
        fail("could not create render targets: " + m_device->lastError());
        return;
    }

    m_stats = {};
    m_stats.drawsSubmitted = static_cast<uint32_t>(m_scene.submitted.size());
    prepareScene(shadowSize);
    m_stats.lights = static_cast<uint32_t>(std::min<size_t>(m_scene.lights.size(), kMaxLights));
    m_stats.debugLines = static_cast<uint32_t>(m_scene.debugVertices.size() / 2);
    m_stats.meshes = m_resources.meshCount();
    m_stats.textures = m_resources.textureCount();
    m_stats.materials = m_resources.materialCount();

    const bool shadowsEnabled = shadowSize > 0;
    const gpu::FrameUniforms_t frame = buildFrameUniforms(m_scene, backbuffer, m_targets, shadowsEnabled);
    const FrameContext_t context{.device = *m_device,
                                 .commands = m_device->commands(),
                                 .shaders = m_shaders,
                                 .resources = m_resources,
                                 .targets = m_targets,
                                 .backbuffer = backbuffer,
                                 .settings = m_settings,
                                 .scene = m_scene,
                                 .frame = frame,
                                 .stats = m_stats,
                                 .shadowsEnabled = shadowsEnabled};
    for (const auto& pass : m_passes) {
        pass->execute(context);
    }
}

void CRenderSystem::endFrame() {
    if (!m_initialized) {
        return;
    }
    if (const auto backbuffer = m_device->beginFrame()) {
        renderFrame(*backbuffer);
        m_device->endFrame();
    }
    m_scene.submitted.clear();
    m_scene.boneMatrices.clear();
    m_scene.lights.clear();
    m_scene.debugVertices.clear();
}

}  // namespace game
