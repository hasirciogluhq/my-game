#include "engine/render/shader_library.hpp"

#include "engine/render/shaders/shader_sources.hpp"

namespace game {
namespace {

using rhi::EShaderStage;

constexpr std::array<ShaderInfo_t, static_cast<size_t>(EShaderId::kCount)> kShaderTable{{
    /* kSceneVertex      */ {EShaderStage::kVertex, "vertex_scene", 2, 0},
    /* kShadowVertex     */ {EShaderStage::kVertex, "vertex_shadow", 2, 0},
    /* kShadowFragment   */ {EShaderStage::kFragment, "fragment_shadow", 0, 0},
    /* kLitFragment      */ {EShaderStage::kFragment, "fragment_lit", 2, 2},
    /* kRimLitFragment   */ {EShaderStage::kFragment, "fragment_rimlit", 2, 2},
    /* kUnlitFragment    */ {EShaderStage::kFragment, "fragment_unlit", 2, 2},
    /* kFullscreenVertex */ {EShaderStage::kVertex, "vertex_fullscreen", 0, 0},
    /* kSkyFragment      */ {EShaderStage::kFragment, "fragment_sky", 1, 0},
    /* kPostFragment     */ {EShaderStage::kFragment, "fragment_post", 1, 1},
    /* kDebugVertex      */ {EShaderStage::kVertex, "vertex_debug", 1, 0},
    /* kDebugFragment    */ {EShaderStage::kFragment, "fragment_debug", 0, 0},
}};

}  // namespace

const ShaderInfo_t& shaderInfo(EShaderId id) {
    return kShaderTable[static_cast<size_t>(id)];
}

std::string_view shaderSource(rhi::EShaderLanguage language) {
    switch (language) {
        case rhi::EShaderLanguage::kMsl: return mslShaderSource();
        case rhi::EShaderLanguage::kHlsl:
        case rhi::EShaderLanguage::kSpirv: break;
    }
    return {};
}

rhi::ShaderHandle_t CShaderLibrary::get(EShaderId id) {
    rhi::ShaderHandle_t& handle = m_shaders[static_cast<size_t>(id)];
    if (handle.valid() || m_device == nullptr) {
        return handle;
    }
    const ShaderInfo_t& info = shaderInfo(id);
    const rhi::EShaderLanguage language = m_device->caps().shaderLanguage;
    handle = m_device->createShader({.stage = info.stage,
                                     .language = language,
                                     .source = shaderSource(language),
                                     .entryPoint = info.entryPoint,
                                     .uniformBufferCount = info.uniformBuffers,
                                     .samplerCount = info.samplers,
                                     .debugName = info.entryPoint});
    return handle;
}

void CShaderLibrary::destroy() {
    if (m_device != nullptr) {
        for (rhi::ShaderHandle_t& handle : m_shaders) {
            m_device->destroy(handle);
            handle = {};
        }
    }
}

}  // namespace game
