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

// Atlas içindeki tek bir harf. x0..y1: atlas pikselleri. xoff/yoff: imleçten harfin sol-üstüne piksel ofseti (y aşağı).
struct Glyph_t {
    float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
    float xoff = 0.0f, yoff = 0.0f, xoff2 = 0.0f, yoff2 = 0.0f;
    float advance = 0.0f;
};

// Tek kanallı (R8) font atlası. pixels -> ETextureFormat::kR8Unorm olarak renderer'a yüklenir.
struct FontAtlas_t {
    uint32_t width = 0;
    uint32_t height = 0;
    float pixelHeight = 0.0f;    // atlasın pişirildiği harf yüksekliği (piksel)
    float ascent = 0.0f;         // taban çizgisinin üstü (piksel)
    float descent = 0.0f;        // taban çizgisinin altı (piksel, negatif)
    float lineHeight = 0.0f;
    std::vector<std::byte> pixels;
    std::unordered_map<uint32_t, Glyph_t> glyphs;  // unicode kod noktası -> harf
};

// Hizalama: yazının pozisyon noktasına göre yatay yeri.
enum class ETextAlign : uint8_t { kCenter, kLeft };

namespace font {

// TTF baytlarından atlas üretir. ASCII + Türkçe + Latin-1 harfleri pişirilir.
[[nodiscard]] ENGINE_API std::expected<FontAtlas_t, std::string> bake(std::span<const std::byte> ttf, float pixelHeight);

// "fonts/DejaVuSans.ttf" gibi assets-göreli yoldan okur ve pişirir.
[[nodiscard]] ENGINE_API std::expected<FontAtlas_t, std::string> load(const std::string& assetPath, float pixelHeight);

// UTF-8 yazıyı quad'lara çevirir. Mesh XY düzleminde, normal +Z, y yukarı.
// worldHeight: bir satırın dünya birimi yüksekliği (metre). Merkez/sol hizası pozisyonun orijinidir.
// Atlasta olmayan harf atlanır. Boş sonuçta vertices boş döner.
[[nodiscard]] ENGINE_API MeshData_t buildTextMesh(const FontAtlas_t& atlas, std::string_view utf8, float worldHeight,
                                                  ETextAlign align = ETextAlign::kCenter);

// Yazının piksel genişliği (atlasta olmayan harfler sayılmaz).
[[nodiscard]] ENGINE_API float measure(const FontAtlas_t& atlas, std::string_view utf8);

// maxWidth pikselin içine sığan en uzun önek: bayt sayısı döner (UTF-8 dizisini ortadan bölmez, en az 0).
[[nodiscard]] ENGINE_API size_t fit(const FontAtlas_t& atlas, std::string_view utf8, float maxWidth);

// Kameraya bakan model matrisi: mesh'in +Z yüzü kameraya döner. scale: ek ölçek (1 = olduğu gibi).
[[nodiscard]] ENGINE_API glm::mat4 billboardModel(const CCamera& camera, const glm::vec3& position, float scale = 1.0f);

// Ekranda sabit boyutlu görünsün diye: o noktada ekran yüksekliğinin dünya karşılığı.
// billboardModel'e scale ver; mesh'in worldHeight'ı artık "ekran yüksekliğinin kesri" olur (örn. 0.03).
[[nodiscard]] ENGINE_API float constantScreenScale(const CCamera& camera, const glm::vec3& position);

}  // namespace font
}  // namespace game
