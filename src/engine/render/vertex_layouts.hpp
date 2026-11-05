#pragma once

#include "engine/render/rhi/rhi_types.hpp"

namespace game {

// Public EVertexLayout -> RHI vertex layout. New layout: a value in EVertexLayout + a case here + a shader input.
[[nodiscard]] inline rhi::VertexLayout_t rhiVertexLayout(EVertexLayout layout) {
    using rhi::EVertexFormat;
    switch (layout) {
        case EVertexLayout::kPosNormalUv:
            return {.stride = 32,
                    .attributeCount = 3,
                    .attributes = {{{.location = 0, .format = EVertexFormat::kFloat3, .offset = 0},
                                    {.location = 1, .format = EVertexFormat::kFloat3, .offset = 12},
                                    {.location = 2, .format = EVertexFormat::kFloat2, .offset = 24}}}};
        case EVertexLayout::kPosNormalUvSkin:
            return {.stride = 52,
                    .attributeCount = 5,
                    .attributes = {{{.location = 0, .format = EVertexFormat::kFloat3, .offset = 0},
                                    {.location = 1, .format = EVertexFormat::kFloat3, .offset = 12},
                                    {.location = 2, .format = EVertexFormat::kFloat2, .offset = 24},
                                    {.location = 3, .format = EVertexFormat::kUByte4Norm, .offset = 32},  // shader: round(x * 255) = joint
                                    {.location = 4, .format = EVertexFormat::kFloat4, .offset = 36}}}};
    }
    return {};
}


// Debug lines: float3 position + float4 color (gpu::DebugVertex_t).
[[nodiscard]] inline rhi::VertexLayout_t debugVertexLayout() {
    using rhi::EVertexFormat;
    return {.stride = 28,
            .attributeCount = 2,
            .attributes = {{{.location = 0, .format = EVertexFormat::kFloat3, .offset = 0},
                            {.location = 1, .format = EVertexFormat::kFloat4, .offset = 12}}}};
}

}  // namespace game
