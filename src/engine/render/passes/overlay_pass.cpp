#include "engine/render/passes/overlay_pass.hpp"

namespace game {

bool COverlayPass::create(const PassInitContext_t&) {
    return true;
}

void COverlayPass::destroy(rhi::IRhiDevice&) {}

void COverlayPass::execute(const FrameContext_t& context) {
    if (context.scene.overlay == nullptr) {
        return;
    }
    rhi::RenderPassDesc_t pass{.colorCount = 1, .debugName = "overlay"};
    pass.colors[0] = {.texture = context.backbuffer.backbuffer, .load = rhi::ELoadOp::kLoad, .store = rhi::EStoreOp::kStore};
    context.commands.recordNative(pass, context.scene.overlay, context.scene.overlayUser);
    ++context.stats.passes;
}

}  // namespace game
