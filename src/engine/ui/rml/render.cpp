#include "engine/ui/rml/render.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <span>

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_TGA
#include <stb_image.h>

#include "engine/engine/core/asset_path.hpp"
#include "engine/interface/i_log.hpp"

namespace game::rml {

namespace {

static_assert(sizeof(Rml::Vertex) == 20,
              "Rml::Vertex layout must match the vertex definition");

// Vertex positions are in pixel space; matrix = ortho(pixel -> NDC) * CSS
// transform * translate, prepared on the CPU.
constexpr char kShaderSource[] = R"(
#include <metal_stdlib>
using namespace metal;

struct VertexIn_t {
    float2 position [[attribute(0)]];
    float4 color [[attribute(1)]];
    float2 uv [[attribute(2)]];
};
struct VertexOut_t {
    float4 position [[position]];
    float4 color;
    float2 uv;
};
struct Uniforms_t {
    float4x4 matrix;
};

vertex VertexOut_t rml_vertex(VertexIn_t in [[stage_in]], constant Uniforms_t& u [[buffer(0)]]) {
    VertexOut_t out;
    out.position = u.matrix * float4(in.position, 0.0, 1.0);
    out.color = in.color;
    out.uv = in.uv;
    return out;
}

fragment float4 rml_fragment(VertexOut_t in [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) {
    return tex.sample(smp, in.uv) * in.color;  // RmlUi color and texture are already alpha-premultiplied
}
)";

SDL_GPUShader *createShader(SDL_GPUDevice *device, SDL_GPUShaderStage stage,
                            const char *entry, uint32_t uniforms,
                            uint32_t samplers) {
  SDL_GPUShaderCreateInfo info{};
  info.code = reinterpret_cast<const Uint8 *>(kShaderSource);
  info.code_size = sizeof(kShaderSource) - 1;
  info.entrypoint = entry;
  info.format = SDL_GPU_SHADERFORMAT_MSL;
  info.stage = stage;
  info.num_uniform_buffers = uniforms;
  info.num_samplers = samplers;
  return SDL_CreateGPUShader(device, &info);
}

SDL_GPUGraphicsPipeline *createPipeline(SDL_GPUDevice *device,
                                        SDL_Window *window) {
  if ((SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_MSL) == 0) {
    GAME_LOG_ERROR("ui", "MSL shader support is required for RmlUi drawing");
    return nullptr;
  }
  SDL_GPUShader *vertex =
      createShader(device, SDL_GPU_SHADERSTAGE_VERTEX, "rml_vertex", 1, 0);
  SDL_GPUShader *fragment =
      createShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, "rml_fragment", 0, 1);
  if (vertex == nullptr || fragment == nullptr) {
    GAME_LOG_ERROR("ui", "RmlUi shader could not be compiled: {}", SDL_GetError());
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    return nullptr;
  }

  const SDL_GPUVertexBufferDescription vertexBuffer{
      .slot = 0,
      .pitch = sizeof(Rml::Vertex),
      .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX};
  const std::array<SDL_GPUVertexAttribute, 3> attributes{{
      {.location = 0,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
       .offset = offsetof(Rml::Vertex, position)},
      {.location = 1,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
       .offset = offsetof(Rml::Vertex, colour)},
      {.location = 2,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
       .offset = offsetof(Rml::Vertex, tex_coord)},
  }};
  // Premultiplied-alpha blending: src*1 + dst*(1-srcAlpha)
  SDL_GPUColorTargetDescription target{};
  target.format = SDL_GetGPUSwapchainTextureFormat(device, window);
  target.blend_state.enable_blend = true;
  target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
  target.blend_state.dst_color_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
  target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
  target.blend_state.dst_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

  SDL_GPUGraphicsPipelineCreateInfo info{};
  info.vertex_shader = vertex;
  info.fragment_shader = fragment;
  info.vertex_input_state.vertex_buffer_descriptions = &vertexBuffer;
  info.vertex_input_state.num_vertex_buffers = 1;
  info.vertex_input_state.vertex_attributes = attributes.data();
  info.vertex_input_state.num_vertex_attributes =
      static_cast<Uint32>(attributes.size());
  info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
  info.rasterizer_state.cull_mode =
      SDL_GPU_CULLMODE_NONE; // RmlUi does not guarantee the winding order
  info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
  info.target_info.color_target_descriptions = &target;
  info.target_info.num_color_targets = 1;

  SDL_GPUGraphicsPipeline *pipeline =
      SDL_CreateGPUGraphicsPipeline(device, &info);
  SDL_ReleaseGPUShader(device, vertex);
  SDL_ReleaseGPUShader(device, fragment);
  if (pipeline == nullptr) {
    GAME_LOG_ERROR("ui", "could not create RmlUi pipeline: {}", SDL_GetError());
  }
  return pipeline;
}

// Creates the transfer buffer and writes the two parts one after the other. On error: nullptr.
SDL_GPUTransferBuffer *makeTransfer(SDL_GPUDevice *device, const void *first,
                                    size_t firstBytes, const void *second,
                                    size_t secondBytes) {
  const SDL_GPUTransferBufferCreateInfo info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = static_cast<Uint32>(firstBytes + secondBytes)};
  SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(device, &info);
  if (transfer == nullptr) {
    return nullptr;
  }
  auto *mapped =
      static_cast<Uint8 *>(SDL_MapGPUTransferBuffer(device, transfer, false));
  if (mapped == nullptr) {
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return nullptr;
  }
  std::memcpy(mapped, first, firstBytes);
  if (secondBytes > 0) {
    std::memcpy(mapped + firstBytes, second, secondBytes);
  }
  SDL_UnmapGPUTransferBuffer(device, transfer);
  return transfer;
}

SDL_GPUBuffer *makeBuffer(SDL_GPUDevice *device, SDL_GPUBufferUsageFlags usage,
                          size_t bytes) {
  const SDL_GPUBufferCreateInfo info{.usage = usage,
                                     .size = static_cast<Uint32>(bytes)};
  return SDL_CreateGPUBuffer(device, &info);
}

void premultiply(std::span<uint8_t> rgba) {
  for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
    const uint32_t alpha = rgba[i + 3];
    rgba[i + 0] = static_cast<uint8_t>(rgba[i + 0] * alpha / 255);
    rgba[i + 1] = static_cast<uint8_t>(rgba[i + 1] * alpha / 255);
    rgba[i + 2] = static_cast<uint8_t>(rgba[i + 2] * alpha / 255);
  }
}

} // namespace

bool CRmlRenderer::initialize(SDL_GPUDevice *device, SDL_Window *window) {
  m_device = device;
  m_pipeline = createPipeline(device, window);
  if (m_pipeline == nullptr) {
    return false;
  }
  const SDL_GPUSamplerCreateInfo samplerInfo{
      .min_filter = SDL_GPU_FILTER_LINEAR,
      .mag_filter = SDL_GPU_FILTER_LINEAR,
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
      .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE};
  m_sampler = SDL_CreateGPUSampler(device, &samplerInfo);
  constexpr std::array<uint8_t, 4> kWhite{255, 255, 255, 255};
  m_whiteTexture = createTexture(kWhite.data(), 1, 1); // for untextured geometry
  if (m_sampler == nullptr || m_whiteTexture == 0) {
    GAME_LOG_ERROR("ui", "could not create RmlUi sampler/white texture: {}",
                   SDL_GetError());
    shutdown();
    return false;
  }
  return true;
}

void CRmlRenderer::shutdown() {
  if (m_device == nullptr) {
    return;
  }
  releaseTransfers();
  for (auto &[handle, geometry] : m_geometries) {
    SDL_ReleaseGPUBuffer(m_device, geometry.vertices);
    SDL_ReleaseGPUBuffer(m_device, geometry.indices);
  }
  for (auto &[handle, texture] : m_textures) {
    SDL_ReleaseGPUTexture(m_device, texture);
  }
  m_geometries.clear();
  m_textures.clear();
  m_commands.clear();
  if (m_sampler != nullptr) {
    SDL_ReleaseGPUSampler(m_device, m_sampler);
    m_sampler = nullptr;
  }
  if (m_pipeline != nullptr) {
    SDL_ReleaseGPUGraphicsPipeline(m_device, m_pipeline);
    m_pipeline = nullptr;
  }
  m_whiteTexture = 0;
  m_device = nullptr;
}

void CRmlRenderer::releaseTransfers() {
  for (const PendingGeometry_t &pending : m_pendingGeometry) {
    SDL_ReleaseGPUTransferBuffer(m_device, pending.transfer);
  }
  for (const PendingTexture_t &pending : m_pendingTextures) {
    SDL_ReleaseGPUTransferBuffer(m_device, pending.transfer);
  }
  m_pendingGeometry.clear();
  m_pendingTextures.clear();
}

void CRmlRenderer::beginFrame(glm::ivec2 viewport) {
  m_commands.clear();
  m_viewport = glm::max(viewport, glm::ivec2{1});
  m_projection = glm::mat4{1.0f};
  m_projection[0][0] = 2.0f / static_cast<float>(m_viewport.x);
  m_projection[1][1] = -2.0f / static_cast<float>(m_viewport.y);
  m_projection[3][0] = -1.0f;
  m_projection[3][1] = 1.0f;
  m_transform = glm::mat4{1.0f};
  m_scissorEnabled = false;
}

// --- Upload to the GPU (outside a render pass)
// -----------------------------------------------------------------------------

void CRmlRenderer::prepare(SDL_GPUCommandBuffer *commands) {
  if (m_pendingGeometry.empty() && m_pendingTextures.empty()) {
    return;
  }
  SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
  for (const PendingGeometry_t &pending : m_pendingGeometry) {
    const auto it = m_geometries.find(pending.handle);
    if (it == m_geometries.end()) {
      continue; // released before it was uploaded
    }
    const SDL_GPUTransferBufferLocation vertexSource{
        .transfer_buffer = pending.transfer, .offset = 0};
    const SDL_GPUBufferRegion vertexTarget{.buffer = it->second.vertices,
                                           .offset = 0,
                                           .size = pending.vertexBytes};
    SDL_UploadToGPUBuffer(copy, &vertexSource, &vertexTarget, false);
    const SDL_GPUTransferBufferLocation indexSource{
        .transfer_buffer = pending.transfer, .offset = pending.vertexBytes};
    const SDL_GPUBufferRegion indexTarget{
        .buffer = it->second.indices, .offset = 0, .size = pending.indexBytes};
    SDL_UploadToGPUBuffer(copy, &indexSource, &indexTarget, false);
  }
  for (const PendingTexture_t &pending : m_pendingTextures) {
    const auto it = m_textures.find(pending.handle);
    if (it == m_textures.end()) {
      continue;
    }
    const SDL_GPUTextureTransferInfo source{.transfer_buffer = pending.transfer,
                                            .offset = 0,
                                            .pixels_per_row = pending.width,
                                            .rows_per_layer = pending.height};
    const SDL_GPUTextureRegion target{
        .texture = it->second, .w = pending.width, .h = pending.height, .d = 1};
    SDL_UploadToGPUTexture(copy, &source, &target, false);
  }
  SDL_EndGPUCopyPass(copy);
  releaseTransfers();
}

// --- Drawing (inside a render pass)
// ---------------------------------------------------------------------------------------

void CRmlRenderer::draw(SDL_GPUCommandBuffer *commands,
                        SDL_GPURenderPass *pass) {
  if (m_commands.empty() || m_pipeline == nullptr) {
    return;
  }
  SDL_BindGPUGraphicsPipeline(pass, m_pipeline);
  for (const DrawCommand_t &command : m_commands) {
    const auto geometry = m_geometries.find(command.geometry);
    const auto texture = m_textures.find(command.texture != 0 ? command.texture
                                                              : m_whiteTexture);
    if (geometry == m_geometries.end() || texture == m_textures.end() ||
        command.scissor.z <= 0 || command.scissor.w <= 0) {
      continue;
    }
    const SDL_Rect scissor{command.scissor.x, command.scissor.y,
                           command.scissor.z, command.scissor.w};
    SDL_SetGPUScissor(pass, &scissor);
    SDL_PushGPUVertexUniformData(commands, 0, glm::value_ptr(command.matrix),
                                 sizeof(glm::mat4));

    const SDL_GPUBufferBinding vertexBinding{
        .buffer = geometry->second.vertices, .offset = 0};
    const SDL_GPUBufferBinding indexBinding{.buffer = geometry->second.indices,
                                            .offset = 0};
    const SDL_GPUTextureSamplerBinding textureBinding{
        .texture = texture->second, .sampler = m_sampler};
    SDL_BindGPUVertexBuffers(pass, 0, &vertexBinding, 1);
    SDL_BindGPUIndexBuffer(pass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_BindGPUFragmentSamplers(pass, 0, &textureBinding, 1);
    SDL_DrawGPUIndexedPrimitives(pass, geometry->second.indexCount, 1, 0, 0, 0);
  }
}

// --- geometry
// ------------------------------------------------------------------------------------------------------

Rml::CompiledGeometryHandle
CRmlRenderer::CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                              Rml::Span<const int> indices) {
  const size_t vertexBytes = vertices.size() * sizeof(Rml::Vertex);
  const size_t indexBytes = indices.size() * sizeof(int);
  if (vertexBytes == 0 || indexBytes == 0) {
    return 0;
  }
  Geometry_t geometry{
      .vertices = makeBuffer(m_device, SDL_GPU_BUFFERUSAGE_VERTEX, vertexBytes),
      .indices = makeBuffer(m_device, SDL_GPU_BUFFERUSAGE_INDEX, indexBytes),
      .indexCount = static_cast<uint32_t>(indices.size())};
  SDL_GPUTransferBuffer *transfer = makeTransfer(
      m_device, vertices.data(), vertexBytes, indices.data(), indexBytes);
  if (geometry.vertices == nullptr || geometry.indices == nullptr ||
      transfer == nullptr) {
    GAME_LOG_ERROR("ui", "could not prepare RmlUi geometry for the GPU: {}",
                   SDL_GetError());
    SDL_ReleaseGPUBuffer(m_device, geometry.vertices);
    SDL_ReleaseGPUBuffer(m_device, geometry.indices);
    SDL_ReleaseGPUTransferBuffer(m_device, transfer);
    return 0;
  }
  const Rml::CompiledGeometryHandle handle = m_nextHandle++;
  m_geometries.emplace(handle, geometry);
  m_pendingGeometry.push_back(
      {.handle = handle,
       .transfer = transfer,
       .vertexBytes = static_cast<uint32_t>(vertexBytes),
       .indexBytes = static_cast<uint32_t>(indexBytes)});
  return handle;
}

void CRmlRenderer::RenderGeometry(Rml::CompiledGeometryHandle geometry,
                                  Rml::Vector2f translation,
                                  Rml::TextureHandle texture) {
  const glm::mat4 move = glm::translate(
      glm::mat4{1.0f}, glm::vec3{translation.x, translation.y, 0.0f});
  const glm::ivec4 fullScreen{0, 0, m_viewport.x, m_viewport.y};
  m_commands.push_back({.geometry = geometry,
                        .texture = texture,
                        .matrix = m_projection * m_transform * move,
                        .scissor = m_scissorEnabled ? m_scissor : fullScreen});
}

void CRmlRenderer::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
  const auto it = m_geometries.find(geometry);
  if (it == m_geometries.end()) {
    return;
  }
  SDL_ReleaseGPUBuffer(m_device,
                       it->second.vertices); // SDL defers the actual release until
                                             // the GPU is done using the buffer
  SDL_ReleaseGPUBuffer(m_device, it->second.indices);
  m_geometries.erase(it);
}

// --- texture
// ----------------------------------------------------------------------------------------------------------

Rml::TextureHandle CRmlRenderer::createTexture(const uint8_t *rgbaPremultiplied,
                                               int width, int height) {
  const SDL_GPUTextureCreateInfo info{.type = SDL_GPU_TEXTURETYPE_2D,
                                      .format =
                                          SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                      .width = static_cast<Uint32>(width),
                                      .height = static_cast<Uint32>(height),
                                      .layer_count_or_depth = 1,
                                      .num_levels = 1};
  SDL_GPUTexture *texture = SDL_CreateGPUTexture(m_device, &info);
  SDL_GPUTransferBuffer *transfer =
      makeTransfer(m_device, rgbaPremultiplied,
                   static_cast<size_t>(width) * height * 4, nullptr, 0);
  if (texture == nullptr || transfer == nullptr) {
    GAME_LOG_ERROR("ui", "could not create RmlUi texture: {}", SDL_GetError());
    SDL_ReleaseGPUTexture(m_device, texture);
    SDL_ReleaseGPUTransferBuffer(m_device, transfer);
    return 0;
  }
  const Rml::TextureHandle handle = m_nextHandle++;
  m_textures.emplace(handle, texture);
  m_pendingTextures.push_back({.handle = handle,
                               .transfer = transfer,
                               .width = static_cast<uint32_t>(width),
                               .height = static_cast<uint32_t>(height)});
  return handle;
}

Rml::TextureHandle CRmlRenderer::LoadTexture(Rml::Vector2i &dimensions,
                                             const Rml::String &source) {
  const auto path = findAssetPath(source);
  if (!path) {
    GAME_LOG_WARN("ui", "RmlUi image not found: {}", source);
    return 0;
  }
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_uc *pixels =
      stbi_load(path->string().c_str(), &width, &height, &channels, 4);
  if (pixels == nullptr) {
    GAME_LOG_WARN("ui", "could not read RmlUi image ({}): {}", source,
                  stbi_failure_reason());
    return 0;
  }
  premultiply({pixels, static_cast<size_t>(width) * height * 4});
  const Rml::TextureHandle handle = createTexture(pixels, width, height);
  stbi_image_free(pixels);
  dimensions = {width, height};
  return handle;
}

Rml::TextureHandle
CRmlRenderer::GenerateTexture(Rml::Span<const Rml::byte> source,
                              Rml::Vector2i dimensions) {
  return createTexture(source.data(), dimensions.x,
                       dimensions.y); // RmlUi already provides alpha-premultiplied RGBA
}

void CRmlRenderer::ReleaseTexture(Rml::TextureHandle texture) {
  const auto it = m_textures.find(texture);
  if (it == m_textures.end()) {
    return;
  }
  SDL_ReleaseGPUTexture(m_device, it->second);
  m_textures.erase(it);
}

// --- state
// ---------------------------------------------------------------------------------------------------------

void CRmlRenderer::EnableScissorRegion(bool enable) {
  m_scissorEnabled = enable;
}

void CRmlRenderer::SetScissorRegion(Rml::Rectanglei region) {
  const glm::ivec2 position{region.Position().x, region.Position().y};
  const glm::ivec2 size{region.Size().x, region.Size().y};
  const glm::ivec2 topLeft = glm::clamp(position, glm::ivec2{0}, m_viewport);
  const glm::ivec2 bottomRight =
      glm::clamp(position + size, glm::ivec2{0}, m_viewport);
  m_scissor = {topLeft.x, topLeft.y, bottomRight.x - topLeft.x,
               bottomRight.y - topLeft.y};
}

void CRmlRenderer::SetTransform(const Rml::Matrix4f *transform) {
  m_transform = transform != nullptr
                    ? glm::make_mat4(transform->data())
                    : glm::mat4{1.0f}; // RmlUi default is column-major
}

} // namespace game::rml
