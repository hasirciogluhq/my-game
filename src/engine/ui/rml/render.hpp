#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include <RmlUi/Core/RenderInterface.h>

struct SDL_GPUBuffer;
struct SDL_GPUCommandBuffer;
struct SDL_GPUDevice;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPURenderPass;
struct SDL_GPUSampler;
struct SDL_GPUTexture;
struct SDL_GPUTransferBuffer;
struct SDL_Window;

namespace game::rml {

// RmlUi drawing interface on SDL_GPU (runs in the renderer's overlay gate).
//
// Flow: Context::Render() calls RenderGeometry; these only write into a command
// list (beginFrame clears the list). The renderer calls the overlay twice:
// prepare() (outside a render pass: uploads to the GPU) and draw() (inside the pass:
// draws the commands). Geometry/texture become GPU objects at Compile/Generate time;
// their data is uploaded in the next prepare(). Supported: geometry, texture, scissor, CSS
// transform. Unsupported: clip mask, layer/filter/shader (RmlUi skips them).
// Shaders are MSL (the project's SDL_GPU backend is Metal).
class CRmlRenderer final : public Rml::RenderInterface {
public:
  [[nodiscard]] bool initialize(SDL_GPUDevice *device, SDL_Window *window);
  void shutdown();

  void beginFrame(glm::ivec2 viewport);
  void prepare(SDL_GPUCommandBuffer *commands);
  void draw(SDL_GPUCommandBuffer *commands, SDL_GPURenderPass *pass);

  Rml::CompiledGeometryHandle
  CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                  Rml::Span<const int> indices) override;
  void RenderGeometry(Rml::CompiledGeometryHandle geometry,
                      Rml::Vector2f translation,
                      Rml::TextureHandle texture) override;
  void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

  Rml::TextureHandle LoadTexture(Rml::Vector2i &dimensions,
                                 const Rml::String &source) override;
  Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source,
                                     Rml::Vector2i dimensions) override;
  void ReleaseTexture(Rml::TextureHandle texture) override;

  void EnableScissorRegion(bool enable) override;
  void SetScissorRegion(Rml::Rectanglei region) override;
  void SetTransform(const Rml::Matrix4f *transform) override;

private:
  struct Geometry_t {
    SDL_GPUBuffer *vertices = nullptr;
    SDL_GPUBuffer *indices = nullptr;
    uint32_t indexCount = 0;
  };
  struct PendingGeometry_t {
    Rml::CompiledGeometryHandle handle = 0;
    SDL_GPUTransferBuffer *transfer = nullptr;
    uint32_t vertexBytes = 0;
    uint32_t indexBytes = 0;
  };
  struct PendingTexture_t {
    Rml::TextureHandle handle = 0;
    SDL_GPUTransferBuffer *transfer = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
  };
  struct DrawCommand_t {
    Rml::CompiledGeometryHandle geometry = 0;
    Rml::TextureHandle texture = 0;
    glm::mat4 matrix{1.0f}; // ortho * transform * translate
    glm::ivec4 scissor{0};  // x, y, width, height (pixels)
  };

  [[nodiscard]] Rml::TextureHandle
  createTexture(const uint8_t *rgbaPremultiplied, int width, int height);
  void releaseTransfers();

  SDL_GPUDevice *m_device = nullptr;
  SDL_GPUGraphicsPipeline *m_pipeline = nullptr;
  SDL_GPUSampler *m_sampler = nullptr;
  Rml::TextureHandle m_whiteTexture = 0;

  std::unordered_map<Rml::CompiledGeometryHandle, Geometry_t> m_geometries;
  std::unordered_map<Rml::TextureHandle, SDL_GPUTexture *> m_textures;
  uintptr_t m_nextHandle = 1;

  std::vector<PendingGeometry_t> m_pendingGeometry;
  std::vector<PendingTexture_t> m_pendingTextures;
  std::vector<DrawCommand_t> m_commands;

  glm::ivec2 m_viewport{1, 1};
  glm::mat4 m_projection{1.0f};
  glm::mat4 m_transform{1.0f};
  bool m_scissorEnabled = false;
  glm::ivec4 m_scissor{0};
};

} // namespace game::rml
