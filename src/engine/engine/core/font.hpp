#pragma once

#include "engine/engine/api.hpp"
#include "engine/engine/core/camera.hpp"
#include "engine/engine/core/types.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace game {

// A single letter in the atlas. x0..y1: atlas pixels. xoff/yoff: pixel offset from the cursor to the glyph's top-left (y down).
struct Glyph_t {
    float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
    float xoff = 0.0f, yoff = 0.0f, xoff2 = 0.0f, yoff2 = 0.0f;
    float advance = 0.0f;
};

// Single-channel (R8) font atlas. pixels are loaded into the renderer as ETextureFormat::kR8Unorm.
struct FontAtlas_t {
    uint32_t width = 0;
    uint32_t height = 0;
    float pixelHeight = 0.0f;    // letter height the atlas was baked at (pixels)
    float ascent = 0.0f;         // above the baseline (pixels)
    float descent = 0.0f;        // below the baseline (pixels, negative)
    float lineHeight = 0.0f;
    std::vector<std::byte> pixels;
    std::unordered_map<uint32_t, Glyph_t> glyphs;  // unicode code point -> glyph
};

// Alignment: horizontal position of the text relative to the position point.
enum class ETextAlign : uint8_t { kCenter, kLeft };

namespace font {

// Bakes an atlas from TTF bytes. ASCII, Turkish and Latin-1 glyphs are baked.
[[nodiscard]] ENGINE_API std::expected<FontAtlas_t, std::string> bake(std::span<const std::byte> ttf, float pixelHeight);

// Reads and bakes from an assets-relative path such as "fonts/DejaVuSans.ttf".
[[nodiscard]] ENGINE_API std::expected<FontAtlas_t, std::string> load(const std::string& assetPath, float pixelHeight);

// Converts UTF-8 text to quads. The mesh lies in the XY plane, normal +Z, y up.
// worldHeight: height of one line in world units (meters). Center/left alignment uses the position origin.
// Glyphs missing from the atlas are skipped. For an empty result, vertices is returned empty.
[[nodiscard]] ENGINE_API MeshData_t buildTextMesh(const FontAtlas_t& atlas, std::string_view utf8, float worldHeight,
                                                  ETextAlign align = ETextAlign::kCenter);

// Pixel width of the text (glyphs missing from the atlas are not counted).
[[nodiscard]] ENGINE_API float measure(const FontAtlas_t& atlas, std::string_view utf8);

// The longest prefix that fits within maxWidth pixels: returns its byte count (does not split a UTF-8 sequence, at least 0).
[[nodiscard]] ENGINE_API size_t fit(const FontAtlas_t& atlas, std::string_view utf8, float maxWidth);

// Model matrix that faces the camera: the mesh's +Z face turns toward the camera. scale: extra scale (1 = as is).
[[nodiscard]] ENGINE_API glm::mat4 billboardModel(const CCamera& camera, const glm::vec3& position, float scale = 1.0f);

// To keep a constant on-screen size: the world-space equivalent of the screen height at that point.
// Pass scale to billboardModel; the mesh's worldHeight then becomes a "fraction of the screen height" (e.g. 0.03).
[[nodiscard]] ENGINE_API float constantScreenScale(const CCamera& camera, const glm::vec3& position);

}  // namespace font
}  // namespace game
