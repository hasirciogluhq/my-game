#pragma once

#include "engine/render/rhi/rhi_device.hpp"

#include <array>
#include <string_view>

namespace game {

// Backend-independent shader identities. Stage / entry name / resource counts are language-independent;
// the source code comes from the file under shaders/ for the language (currently only MSL).
enum class EShaderId : uint8_t {
    kSceneVertex,
    kShadowVertex,
    kShadowFragment,
    kLitFragment,
    kRimLitFragment,
    kUnlitFragment,
    kFullscreenVertex,
    kSkyFragment,
    kPostFragment,
    kDebugVertex,
    kDebugFragment,
    kSceneSkinnedVertex,   // vertex_scene + bone palette (vertex b2)
    kShadowSkinnedVertex,  // vertex_shadow + bone palette (vertex b2)
    kCount,
};

struct ShaderInfo_t {
    rhi::EShaderStage stage;
    const char* entryPoint;
    uint32_t uniformBuffers;
    uint32_t samplers;
};

[[nodiscard]] const ShaderInfo_t& shaderInfo(EShaderId id);

// Source containing all shaders for a language. Empty if the language is not supported.
[[nodiscard]] std::string_view shaderSource(rhi::EShaderLanguage language);

// Shader objects are created on first request and stored.
class CShaderLibrary {
public:
    void bind(rhi::IRhiDevice* device) { m_device = device; }
    [[nodiscard]] rhi::ShaderHandle_t get(EShaderId id);
    void destroy();

private:
    rhi::IRhiDevice* m_device = nullptr;
    std::array<rhi::ShaderHandle_t, static_cast<size_t>(EShaderId::kCount)> m_shaders{};
};

}  // namespace game
