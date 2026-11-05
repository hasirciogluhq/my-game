#pragma once

#include "engine/render/rhi/rhi_device.hpp"

#include <array>
#include <string_view>

namespace game {

// Backend'den bağımsız shader kimlikleri. Stage / giriş adı / kaynak sayıları dilden bağımsızdır;
// kaynak kodu dile göre shaders/ altındaki dosyadan gelir (şimdilik sadece MSL).
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
    kCount,
};

struct ShaderInfo_t {
    rhi::EShaderStage stage;
    const char* entryPoint;
    uint32_t uniformBuffers;
    uint32_t samplers;
};

[[nodiscard]] const ShaderInfo_t& shaderInfo(EShaderId id);

// Dil için tüm shader'ları içeren kaynak. Dil desteklenmiyorsa boş.
[[nodiscard]] std::string_view shaderSource(rhi::EShaderLanguage language);

// Shader nesnelerini ilk istekte oluşturup saklar.
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
