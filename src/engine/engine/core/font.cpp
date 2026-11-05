#include "engine/engine/core/font.hpp"

#include "engine/engine/core/asset_path.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>

#include <glm/geometric.hpp>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace game::font {

namespace {

constexpr uint32_t kAtlasSize = 1024;
constexpr uint32_t kFirstAscii = 32;
constexpr uint32_t kLastAscii = 126;
constexpr uint32_t kFirstLatin1 = 0xA0;
constexpr uint32_t kLastLatin1 = 0xFF;
constexpr std::array<uint32_t, 6> kTurkish{0x011E, 0x011F, 0x0130, 0x0131, 0x015E, 0x015F};  // Ğ ğ İ ı Ş ş

std::vector<int> bakedCodepoints() {
    std::vector<int> codepoints;
    for (uint32_t c = kFirstAscii; c <= kLastAscii; ++c) codepoints.push_back(static_cast<int>(c));
    for (uint32_t c = kFirstLatin1; c <= kLastLatin1; ++c) codepoints.push_back(static_cast<int>(c));
    for (uint32_t c : kTurkish) codepoints.push_back(static_cast<int>(c));
    return codepoints;
}

Glyph_t toGlyph(const stbtt_packedchar& packed) {
    return Glyph_t{
        .x0 = static_cast<float>(packed.x0), .y0 = static_cast<float>(packed.y0),
        .x1 = static_cast<float>(packed.x1), .y1 = static_cast<float>(packed.y1),
        .xoff = packed.xoff, .yoff = packed.yoff, .xoff2 = packed.xoff2, .yoff2 = packed.yoff2,
        .advance = packed.xadvance,
    };
}

// UTF-8 dizisinin ilk baytından uzunluğu. Geçersiz bayt: 0.
size_t sequenceLength(uint8_t lead) {
    return lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2 : (lead >> 4) == 0xE ? 3 : (lead >> 3) == 0x1E ? 4 : 0;
}

// UTF-8 -> kod noktaları. Bozuk bayt dizisinde o bayt atlanır.
std::vector<uint32_t> decodeUtf8(std::string_view text) {
    std::vector<uint32_t> out;
    for (size_t i = 0; i < text.size();) {
        const auto lead = static_cast<uint8_t>(text[i]);
        const size_t length = sequenceLength(lead);
        if (length == 0 || i + length > text.size()) {
            ++i;
            continue;
        }
        uint32_t codepoint = length == 1 ? lead : lead & (0xFFu >> (length + 1));
        for (size_t k = 1; k < length; ++k) codepoint = (codepoint << 6) | (static_cast<uint8_t>(text[i + k]) & 0x3F);
        out.push_back(codepoint);
        i += length;
    }
    return out;
}

// Satır genişliği (piksel): sadece atlasta olan harflerin advance toplamı.
float lineWidth(const FontAtlas_t& atlas, std::span<const uint32_t> codepoints) {
    float width = 0.0f;
    for (uint32_t c : codepoints) {
        if (auto it = atlas.glyphs.find(c); it != atlas.glyphs.end()) width += it->second.advance;
    }
    return width;
}

void addQuad(MeshData_t& mesh, const FontAtlas_t& atlas, const Glyph_t& glyph, float cursorX, float pixelToWorld) {
    // Atlas y aşağı, dünya y yukarı. Taban çizgisi y=0.
    const float left = (cursorX + glyph.xoff) * pixelToWorld;
    const float right = (cursorX + glyph.xoff2) * pixelToWorld;
    const float top = -glyph.yoff * pixelToWorld;
    const float bottom = -glyph.yoff2 * pixelToWorld;
    const float invW = 1.0f / static_cast<float>(atlas.width);
    const float invH = 1.0f / static_cast<float>(atlas.height);
    const auto base = static_cast<uint32_t>(mesh.vertices.size());
    const glm::vec3 normal{0.0f, 0.0f, 1.0f};
    mesh.vertices.push_back({{left, bottom, 0.0f}, normal, {glyph.x0 * invW, glyph.y1 * invH}});
    mesh.vertices.push_back({{right, bottom, 0.0f}, normal, {glyph.x1 * invW, glyph.y1 * invH}});
    mesh.vertices.push_back({{right, top, 0.0f}, normal, {glyph.x1 * invW, glyph.y0 * invH}});
    mesh.vertices.push_back({{left, top, 0.0f}, normal, {glyph.x0 * invW, glyph.y0 * invH}});
    for (uint32_t index : {0u, 1u, 2u, 0u, 2u, 3u}) mesh.indices.push_back(base + index);
}

}  // namespace

std::expected<FontAtlas_t, std::string> bake(std::span<const std::byte> ttf, float pixelHeight) {
    if (ttf.empty() || pixelHeight <= 0.0f) {
        return std::unexpected("font: bos veri ya da gecersiz yukseklik");
    }
    const auto* fontData = reinterpret_cast<const unsigned char*>(ttf.data());
    stbtt_fontinfo info;
    if (stbtt_InitFont(&info, fontData, stbtt_GetFontOffsetForIndex(fontData, 0)) == 0) {
        return std::unexpected("font: gecerli bir TTF degil");
    }

    FontAtlas_t atlas;
    atlas.width = kAtlasSize;
    atlas.height = kAtlasSize;
    atlas.pixelHeight = pixelHeight;
    atlas.pixels.assign(static_cast<size_t>(kAtlasSize) * kAtlasSize, std::byte{0});

    std::vector<int> codepoints = bakedCodepoints();
    std::vector<stbtt_packedchar> packed(codepoints.size());
    stbtt_pack_range range{};
    range.font_size = pixelHeight;
    range.array_of_unicode_codepoints = codepoints.data();
    range.num_chars = static_cast<int>(codepoints.size());
    range.chardata_for_range = packed.data();

    stbtt_pack_context context;
    if (stbtt_PackBegin(&context, reinterpret_cast<unsigned char*>(atlas.pixels.data()), kAtlasSize, kAtlasSize, 0, 1, nullptr) == 0) {
        return std::unexpected("font: atlas baslatilamadi");
    }
    stbtt_PackSetOversampling(&context, 2, 2);  // daha yumusak kenar, buyutunce daha az blok
    const int packedOk = stbtt_PackFontRanges(&context, fontData, 0, &range, 1);
    stbtt_PackEnd(&context);
    if (packedOk == 0) {
        return std::unexpected("font: harfler atlasa sigmadi (atlas boyutunu ya da pixelHeight'i kucult)");
    }

    for (size_t i = 0; i < codepoints.size(); ++i) {
        atlas.glyphs.emplace(static_cast<uint32_t>(codepoints[i]), toGlyph(packed[i]));
    }

    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    const float scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);
    atlas.ascent = static_cast<float>(ascent) * scale;
    atlas.descent = static_cast<float>(descent) * scale;
    atlas.lineHeight = static_cast<float>(ascent - descent + lineGap) * scale;
    return atlas;
}

std::expected<FontAtlas_t, std::string> load(const std::string& assetPath, float pixelHeight) {
    const auto path = findAssetPath(assetPath);
    if (!path) {
        return std::unexpected("font: dosya bulunamadi: " + assetPath);
    }
    std::ifstream file(*path, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return bake(std::as_bytes(std::span{bytes}), pixelHeight);
}

MeshData_t buildTextMesh(const FontAtlas_t& atlas, std::string_view utf8, float worldHeight, ETextAlign align) {
    MeshData_t mesh;
    mesh.name = "text";
    const std::vector<uint32_t> codepoints = decodeUtf8(utf8);
    const float pixelToWorld = worldHeight / atlas.pixelHeight;
    float cursorX = align == ETextAlign::kCenter ? -lineWidth(atlas, codepoints) * 0.5f : 0.0f;
    for (uint32_t c : codepoints) {
        const auto it = atlas.glyphs.find(c);
        if (it == atlas.glyphs.end()) continue;
        const bool isSpace = c == U' ' || c == 0xA0;  // boşluk: quad üretmeden sadece ilerle
        if (!isSpace) addQuad(mesh, atlas, it->second, cursorX, pixelToWorld);
        cursorX += it->second.advance;
    }
    return mesh;
}

float measure(const FontAtlas_t& atlas, std::string_view utf8) {
    return lineWidth(atlas, decodeUtf8(utf8));
}

size_t fit(const FontAtlas_t& atlas, std::string_view utf8, float maxWidth) {
    float width = 0.0f;
    size_t i = 0;
    while (i < utf8.size()) {
        const size_t length = std::max<size_t>(sequenceLength(static_cast<uint8_t>(utf8[i])), 1);
        const size_t end = std::min(i + length, utf8.size());
        const std::vector<uint32_t> codepoint = decodeUtf8(utf8.substr(i, end - i));
        width += lineWidth(atlas, codepoint);
        if (width > maxWidth) {
            break;
        }
        i = end;
    }
    return i;
}

glm::mat4 billboardModel(const CCamera& camera, const glm::vec3& position, float scale) {
    const glm::vec3 forward = camera.forward();
    const glm::vec3 right = camera.right();
    const glm::vec3 up = glm::cross(right, forward);
    return glm::mat4{glm::vec4{right * scale, 0.0f}, glm::vec4{up * scale, 0.0f}, glm::vec4{-forward * scale, 0.0f},
                     glm::vec4{position, 1.0f}};
}

float constantScreenScale(const CCamera& camera, const glm::vec3& position) {
    // Projeksiyon [1][1] = 1/tan(fov/2): mesafe d'de ekran yuksekligi 2*d/[1][1] dunya birimidir.
    const float distance = glm::dot(position - camera.position(), camera.forward());
    return std::max(distance, 0.1f) * 2.0f / camera.projection()[1][1];
}

}  // namespace game::font
