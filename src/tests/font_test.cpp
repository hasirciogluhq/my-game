// Başsız font testi: atlas pişirme + metin mesh'i + billboard. Çalıştır: xmake run font_test
#include "engine/engine/core/camera.hpp"
#include "engine/engine/core/font.hpp"

#include <algorithm>
#include <iostream>

namespace {

int g_failures = 0;

void check(bool ok, const char* what) {
    std::cout << (ok ? "ok    " : "FAIL  ") << what << "\n";
    g_failures += ok ? 0 : 1;
}

}  // namespace

int main() {
    using namespace game;
    auto atlas = font::load("fonts/DejaVuSans.ttf", 48.0f);
    check(atlas.has_value(), "atlas pisti");
    if (!atlas) {
        std::cout << atlas.error() << "\n";
        return 1;
    }
    check(std::ranges::any_of(atlas->pixels, [](std::byte b) { return b != std::byte{0}; }), "atlas bos degil");
    check(atlas->glyphs.contains(U'ğ') && atlas->glyphs.contains(U'İ') && atlas->glyphs.contains(U'ş'), "turkce harfler var");

    const MeshData_t mesh = font::buildTextMesh(*atlas, "Işık ğüş", 0.3f);
    check(mesh.vertices.size() == 7 * 4 && mesh.indices.size() == 7 * 6, "7 harf = 28 vertex, 42 index");
    check(font::buildTextMesh(*atlas, "", 0.3f).vertices.empty(), "bos yazi bos mesh");

    CCamera camera;
    camera.setPerspective(70.0f, 16.0f / 9.0f, 0.1f, 200.0f);
    camera.setPosition({0.0f, 1.6f, 0.0f});
    const glm::mat4 model = font::billboardModel(camera, {0.0f, 2.0f, -5.0f});
    check(model[3].z == -5.0f && model[2].z > 0.99f, "billboard +Z kameraya bakar (yaw=0)");
    check(font::constantScreenScale(camera, {0.0f, 2.0f, -5.0f}) > 0.0f, "sabit ekran olcegi pozitif");

    std::cout << (g_failures == 0 ? "TUM TESTLER GECTI" : "BAZI TESTLER BASARISIZ") << "\n";
    return g_failures == 0 ? 0 : 1;
}
