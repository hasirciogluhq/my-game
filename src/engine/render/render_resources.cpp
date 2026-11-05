#include "engine/render/render_resources.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <utility>

#include <glm/common.hpp>

namespace game {
namespace {

rhi::EPixelFormat toRhiFormat(ETextureFormat format) {
    switch (format) {
        case ETextureFormat::kR8Unorm: return rhi::EPixelFormat::kR8Unorm;
        case ETextureFormat::kRgba8Unorm: return rhi::EPixelFormat::kRgba8Unorm;
        case ETextureFormat::kRgba8UnormSrgb: return rhi::EPixelFormat::kRgba8UnormSrgb;
        case ETextureFormat::kRgba16Float: return rhi::EPixelFormat::kRgba16Float;
    }
    return rhi::EPixelFormat::kUndefined;
}

// Position her layout'ta vertex'in başındaki float3'tür.
void computeBounds(std::span<const std::byte> vertices, uint32_t stride, glm::vec3& boundsMin, glm::vec3& boundsMax) {
    boundsMin = glm::vec3{std::numeric_limits<float>::max()};
    boundsMax = glm::vec3{std::numeric_limits<float>::lowest()};
    for (size_t offset = 0; offset + stride <= vertices.size(); offset += stride) {
        glm::vec3 position;
        std::memcpy(&position, vertices.data() + offset, sizeof(position));
        boundsMin = glm::min(boundsMin, position);
        boundsMax = glm::max(boundsMax, position);
    }
}

MaterialDesc_t sanitized(const MaterialDesc_t& desc) {
    MaterialDesc_t copy = desc;
    copy.debugName = "";  // çağıranın string'i çağrıdan sonra yaşamayabilir
    return copy;
}

}  // namespace

bool CRenderResources::fail(std::string message) {
    m_lastError = std::move(message);
    std::cerr << "render: " << m_lastError << "\n";
    return false;
}

bool CRenderResources::initialize(rhi::IRhiDevice& device) {
    m_device = &device;

    // Gölge: karşılaştırmayı shader yapar, filtre yok.
    m_shadowSampler = device.createSampler({.minFilter = rhi::EFilter::kNearest, .magFilter = rhi::EFilter::kNearest, .mipFilter = rhi::EFilter::kNearest});
    m_linearClampSampler = device.createSampler({});
    m_materialSampler = device.createSampler({.addressU = rhi::EAddressMode::kRepeat,
                                              .addressV = rhi::EAddressMode::kRepeat,
                                              .addressW = rhi::EAddressMode::kRepeat,
                                              .maxAnisotropy = 8.0f});
    if (!m_shadowSampler.valid() || !m_linearClampSampler.valid() || !m_materialSampler.valid()) {
        return fail("sampler oluşturulamadı: " + device.lastError());
    }

    constexpr std::array<std::byte, 4> kWhitePixel{std::byte{255}, std::byte{255}, std::byte{255}, std::byte{255}};
    m_whiteTexture = createTexture({.debugName = "white", .width = 1, .height = 1, .format = ETextureFormat::kRgba8UnormSrgb,
                                    .generateMips = false, .pixels = kWhitePixel});
    m_defaultMaterial = createMaterial({.debugName = "default"});
    if (!m_whiteTexture.valid() || !m_defaultMaterial.valid()) {
        return fail("varsayılan kaynaklar oluşturulamadı");
    }
    return true;
}

void CRenderResources::shutdown() {
    if (m_device == nullptr) {
        return;
    }
    m_meshes.drain([this](GpuMesh_t& mesh) {
        m_device->destroy(mesh.vertexBuffer);
        m_device->destroy(mesh.indexBuffer);
    });
    m_textures.drain([this](GpuTexture_t& texture) { m_device->destroy(texture.texture); });
    m_materials.drain([](MaterialDesc_t&) {});
    m_device->destroy(m_shadowSampler);
    m_device->destroy(m_linearClampSampler);
    m_device->destroy(m_materialSampler);
    m_shadowSampler = m_linearClampSampler = m_materialSampler = {};
    m_whiteTexture = {};
    m_defaultMaterial = {};
    m_device = nullptr;
}

MeshHandle_t CRenderResources::createMesh(const MeshDesc_t& desc) {
    const uint32_t stride = vertexStride(desc.layout);
    const uint32_t indexBytes = indexSize(desc.indexType);
    if (stride == 0 || desc.vertexData.empty() || desc.vertexData.size() % stride != 0 || desc.indexData.empty() ||
        desc.indexData.size() % indexBytes != 0) {
        fail(std::string{"createMesh: vertex/index verisi geçersiz ("} + desc.debugName + ")");
        return {};
    }

    GpuMesh_t mesh;
    mesh.vertexBuffer = m_device->createBuffer({.sizeBytes = static_cast<uint32_t>(desc.vertexData.size()), .usage = rhi::EBufferUsage::kVertex,
                                                .debugName = desc.debugName}, desc.vertexData);
    mesh.indexBuffer = m_device->createBuffer({.sizeBytes = static_cast<uint32_t>(desc.indexData.size()), .usage = rhi::EBufferUsage::kIndex,
                                               .debugName = desc.debugName}, desc.indexData);
    if (!mesh.vertexBuffer.valid() || !mesh.indexBuffer.valid()) {
        m_device->destroy(mesh.vertexBuffer);
        m_device->destroy(mesh.indexBuffer);
        fail(std::string{"createMesh: GPU buffer oluşturulamadı: "} + desc.debugName);
        return {};
    }
    mesh.indexCount = static_cast<uint32_t>(desc.indexData.size() / indexBytes);
    mesh.indexType = desc.indexType;
    mesh.layout = desc.layout;
    computeBounds(desc.vertexData, stride, mesh.boundsMin, mesh.boundsMax);
    return m_meshes.create(mesh);
}

void CRenderResources::destroyMesh(MeshHandle_t handle) {
    if (auto mesh = m_meshes.destroy(handle)) {
        m_device->destroy(mesh->vertexBuffer);
        m_device->destroy(mesh->indexBuffer);
    }
}

TextureHandle_t CRenderResources::createTexture(const TextureDesc_t& desc) {
    const rhi::TextureDesc_t rhiDesc{.width = desc.width,
                                     .height = desc.height,
                                     .format = toRhiFormat(desc.format),
                                     .usage = rhi::ETextureUsage::kSampled,
                                     .generateMips = desc.generateMips,
                                     .debugName = desc.debugName};
    const rhi::TextureHandle_t texture = m_device->createTexture(rhiDesc, desc.pixels);
    if (!texture.valid()) {
        fail(std::string{"createTexture: "} + m_device->lastError());
        return {};
    }
    return m_textures.create({.texture = texture, .width = desc.width, .height = desc.height});
}

void CRenderResources::destroyTexture(TextureHandle_t handle) {
    if (handle == m_whiteTexture) {
        return;  // varsayılan texture silinemez
    }
    if (auto texture = m_textures.destroy(handle)) {
        m_device->destroy(texture->texture);
    }
}

MaterialHandle_t CRenderResources::createMaterial(const MaterialDesc_t& desc) {
    return m_materials.create(sanitized(desc));
}

void CRenderResources::updateMaterial(MaterialHandle_t handle, const MaterialDesc_t& desc) {
    if (MaterialDesc_t* material = m_materials.get(handle)) {
        *material = sanitized(desc);
    }
}

void CRenderResources::destroyMaterial(MaterialHandle_t handle) {
    if (handle != m_defaultMaterial) {
        (void)m_materials.destroy(handle);
    }
}

rhi::TextureHandle_t CRenderResources::baseTextureFor(const MaterialDesc_t& material) const {
    const GpuTexture_t* texture = m_textures.get(material.baseColorTexture);
    if (texture == nullptr) {
        texture = m_textures.get(m_whiteTexture);
    }
    return texture != nullptr ? texture->texture : rhi::TextureHandle_t{};
}

}  // namespace game
