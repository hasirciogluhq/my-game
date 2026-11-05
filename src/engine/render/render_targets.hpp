#pragma once

#include "engine/render/rhi/rhi_device.hpp"

namespace game {

inline constexpr rhi::EPixelFormat kHdrFormat = rhi::EPixelFormat::kRgba16Float;
inline constexpr rhi::EPixelFormat kDepthFormat = rhi::EPixelFormat::kD32Float;

// Resolution-dependent targets: HDR color + depth (at render scale) and the shadow map.
// Recreated when the size changes (window resize, render scale, shadow quality).
class CRenderTargets {
public:
    // shadowSize 0 means shadows are off; a 1x1 map is kept (the shader always binds a texture).
    [[nodiscard]] bool ensure(rhi::IRhiDevice& device, uint32_t width, uint32_t height, uint32_t shadowSize);
    void release(rhi::IRhiDevice& device);

    [[nodiscard]] rhi::TextureHandle_t hdr() const { return m_hdr; }
    [[nodiscard]] rhi::TextureHandle_t depth() const { return m_depth; }
    [[nodiscard]] rhi::TextureHandle_t shadowMap() const { return m_shadowMap; }
    [[nodiscard]] uint32_t width() const { return m_width; }
    [[nodiscard]] uint32_t height() const { return m_height; }
    [[nodiscard]] uint32_t shadowSize() const { return m_shadowSize; }

private:
    rhi::TextureHandle_t m_hdr;
    rhi::TextureHandle_t m_depth;
    rhi::TextureHandle_t m_shadowMap;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_shadowSize = 0;
};

}  // namespace game
