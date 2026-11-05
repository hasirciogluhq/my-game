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

// Yeni backend eklemek: ilgili IRhiDevice sınıfını yaz, burada bir case ekle, shader kaynağını shaders/ altına ekle.
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
            error = "Metal backend yalnızca macOS'ta mevcut";
            return nullptr;
#endif
        case ERenderBackend::kD3D11:
            error = "D3D11 backend henüz yazılmadı (rhi/d3d11/ altında IRhiDevice implementasyonu gerekir)";
            return nullptr;
        case ERenderBackend::kD3D12:
        case ERenderBackend::kVulkan:
            error = "bu backend için shader kaynağı yok (shaders/ altına HLSL/SPIR-V eklenmeli)";
            return nullptr;
        case ERenderBackend::kAuto:
            break;
    }
    error = "bilinmeyen backend";
    return nullptr;
}

}  // namespace game::rhi
