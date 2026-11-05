#include "engine/render/rhi/rhi_device.hpp"
#include "engine/render/rhi/sdl_gpu_device.hpp"

#include <SDL3/SDL.h>

namespace game::rhi {
namespace {

ERenderBackend defaultBackend() {
#if defined(__APPLE__)
    return ERenderBackend::kMetal;
#elif defined(_WIN32)
    return ERenderBackend::kD3D11;
#else
    return ERenderBackend::kVulkan;
#endif
}

}  // namespace

// To add a new backend: write the matching IRhiDevice class, add a case here, and add its shader source under shaders/.
std::unique_ptr<IRhiDevice> createRhiDevice(ERenderBackend backend, std::string& error) {
    if (backend == ERenderBackend::kAuto) {
        backend = defaultBackend();
    }
    switch (backend) {
        case ERenderBackend::kMetal:
#if defined(__APPLE__)
            return std::make_unique<CSdlGpuDevice>(SdlGpuConfig_t{.shaderFormats = SDL_GPU_SHADERFORMAT_MSL,
                                                                  .driverName = "metal",
                                                                  .language = EShaderLanguage::kMsl,
                                                                  .backend = ERenderBackend::kMetal,
                                                                  .backendName = "SDL_GPU/Metal"});
#else
            error = "the Metal backend is only available on macOS";
            return nullptr;
#endif
        case ERenderBackend::kD3D11:
            error = "the D3D11 backend is not written yet (an IRhiDevice implementation under rhi/d3d11/ is required)";
            return nullptr;
        case ERenderBackend::kD3D12:
        case ERenderBackend::kVulkan:
            error = "there is no shader source for this backend (HLSL/SPIR-V must be added under shaders/)";
            return nullptr;
        case ERenderBackend::kAuto:
            break;
    }
    error = "unknown backend";
    return nullptr;
}

}  // namespace game::rhi
