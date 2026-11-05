#pragma once

#include "engine/interface/render_types.hpp"
#include "engine/render/handle_pool.hpp"
#include "engine/render/rhi/rhi_device.hpp"

#include <string>

namespace game {

struct GpuMesh_t {
    rhi::BufferHandle_t vertexBuffer;
    rhi::BufferHandle_t indexBuffer;
    uint32_t indexCount = 0;
    EIndexType indexType = EIndexType::kUint32;
    EVertexLayout layout = EVertexLayout::kPosNormalUv;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};

struct GpuTexture_t {
    rhi::TextureHandle_t texture;
    uint32_t width = 0;
    uint32_t height = 0;
};

// Mesh / texture / material havuzları, varsayılan texture + material ve ortak sampler'lar.
// find() pointer'ları create*()'e kadar geçerlidir (havuz büyüyebilir).
class CRenderResources {
public:
    [[nodiscard]] bool initialize(rhi::IRhiDevice& device);
    void shutdown();

    [[nodiscard]] MeshHandle_t createMesh(const MeshDesc_t& desc);
    void destroyMesh(MeshHandle_t handle);
    [[nodiscard]] const GpuMesh_t* find(MeshHandle_t handle) const { return m_meshes.get(handle); }

    [[nodiscard]] TextureHandle_t createTexture(const TextureDesc_t& desc);
    void destroyTexture(TextureHandle_t handle);

    [[nodiscard]] MaterialHandle_t createMaterial(const MaterialDesc_t& desc);
    void updateMaterial(MaterialHandle_t handle, const MaterialDesc_t& desc);
    void destroyMaterial(MaterialHandle_t handle);
    [[nodiscard]] const MaterialDesc_t* find(MaterialHandle_t handle) const { return m_materials.get(handle); }

    [[nodiscard]] MaterialHandle_t defaultMaterial() const { return m_defaultMaterial; }
    // Materyalin base color texture'ı; yoksa veya silinmişse beyaz texture.
    [[nodiscard]] rhi::TextureHandle_t baseTextureFor(const MaterialDesc_t& material) const;

    // Public texture handle'ının GPU texture'ı; geçersiz/silinmişse geçersiz handle.
    [[nodiscard]] rhi::TextureHandle_t gpuTexture(TextureHandle_t handle) const {
        const GpuTexture_t* texture = m_textures.get(handle);
        return texture != nullptr ? texture->texture : rhi::TextureHandle_t{};
    }

    [[nodiscard]] rhi::SamplerHandle_t shadowSampler() const { return m_shadowSampler; }
    [[nodiscard]] rhi::SamplerHandle_t linearClampSampler() const { return m_linearClampSampler; }
    [[nodiscard]] rhi::SamplerHandle_t materialSampler() const { return m_materialSampler; }

    [[nodiscard]] uint32_t meshCount() const { return m_meshes.count(); }
    [[nodiscard]] uint32_t textureCount() const { return m_textures.count(); }
    [[nodiscard]] uint32_t materialCount() const { return m_materials.count(); }
    [[nodiscard]] const std::string& lastError() const { return m_lastError; }

private:
    bool fail(std::string message);

    rhi::IRhiDevice* m_device = nullptr;
    CHandlePool<MeshHandle_t, GpuMesh_t> m_meshes;
    CHandlePool<TextureHandle_t, GpuTexture_t> m_textures;
    CHandlePool<MaterialHandle_t, MaterialDesc_t> m_materials;

    TextureHandle_t m_whiteTexture;
    MaterialHandle_t m_defaultMaterial;
    rhi::SamplerHandle_t m_shadowSampler;
    rhi::SamplerHandle_t m_linearClampSampler;
    rhi::SamplerHandle_t m_materialSampler;
    std::string m_lastError;
};

}  // namespace game
