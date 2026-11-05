#include "engine/render/render_targets.hpp"

#include <algorithm>
#include <utility>

namespace game {

bool CRenderTargets::ensure(rhi::IRhiDevice& device, uint32_t width, uint32_t height, uint32_t shadowSize) {
    const uint32_t shadowTexels = std::max(shadowSize, 1u);
    const bool sceneOk = m_hdr.valid() && m_depth.valid() && m_width == width && m_height == height;
    const bool shadowOk = m_shadowMap.valid() && m_shadowSize == shadowTexels;
    if (sceneOk && shadowOk) {
        return true;
    }

    if (!sceneOk) {
        device.destroy(std::exchange(m_hdr, {}));
        device.destroy(std::exchange(m_depth, {}));
        m_hdr = device.createTexture({.width = width, .height = height, .format = kHdrFormat,
                                      .usage = rhi::ETextureUsage::kColorTarget | rhi::ETextureUsage::kSampled, .debugName = "hdr"}, {});
        m_depth = device.createTexture({.width = width, .height = height, .format = kDepthFormat,
                                        .usage = rhi::ETextureUsage::kDepthTarget, .debugName = "depth"}, {});
        m_width = width;
        m_height = height;
    }
    if (!shadowOk) {
        device.destroy(std::exchange(m_shadowMap, {}));
        m_shadowMap = device.createTexture({.width = shadowTexels, .height = shadowTexels, .format = kDepthFormat,
                                            .usage = rhi::ETextureUsage::kDepthTarget | rhi::ETextureUsage::kSampled, .debugName = "shadow"}, {});
        m_shadowSize = shadowTexels;
    }
    if (!m_hdr.valid() || !m_depth.valid() || !m_shadowMap.valid()) {
        release(device);
        return false;
    }
    return true;
}

void CRenderTargets::release(rhi::IRhiDevice& device) {
    device.destroy(std::exchange(m_hdr, {}));
    device.destroy(std::exchange(m_depth, {}));
    device.destroy(std::exchange(m_shadowMap, {}));
    m_width = m_height = m_shadowSize = 0;
}

}  // namespace game
