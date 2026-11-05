#pragma once

#include "engine/engine/api.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// İskelet + skin + animasyon klibi: sadece veri ve pose hesabı. Hangi klibin ne zaman çalacağı OYUN kararıdır
// (game/server/systems/animation); motor sadece "şu klip, şu saniyede" pozunu üretir.
namespace game::anim {

// Eklem sayısı renderer'ın kMaxBones'unu (render_types.hpp) aşamaz; yükleyici aşan modeli reddeder.
// GPU'ya giden vertex: render_types.hpp EVertexLayout::kPosNormalUvSkin (52 bayt) ile BİREBİR aynı düzen.
struct SkinnedVertex_t {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
    std::array<uint8_t, 4> joints{};  // Joint indeksi (SkinnedModel_t::joints)
    glm::vec4 weights{0.0f};          // toplamı 1
};
static_assert(sizeof(SkinnedVertex_t) == 52);

// Sahne ağacındaki bir düğüm. nodes dizisi ebeveyn önce gelecek şekilde sıralıdır.
struct Node_t {
    std::string name;
    int parent = -1;
    glm::vec3 translation{0.0f};  // bind pozundaki yerel dönüşüm
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

// Vertex'in joint indeksinin gösterdiği eklem: hangi düğüm + bind pozunu geri alan matris.
struct Joint_t {
    int node = 0;
    glm::mat4 inverseBind{1.0f};
};

template <typename T>
struct Key_t {
    float time = 0.0f;  // saniye
    T value{};
};

// Bir düğümün animasyonu. Boş track = bind pozu kullanılır.
struct Channel_t {
    int node = 0;
    std::vector<Key_t<glm::vec3>> translations;
    std::vector<Key_t<glm::quat>> rotations;
    std::vector<Key_t<glm::vec3>> scales;
};

struct Clip_t {
    std::string name;
    float duration = 0.0f;  // saniye; klipler döngülüdür
    std::vector<Channel_t> channels;
};

// Çözülmüş (RGBA8, sRGB renk) base color dokusu. Boşsa (pixels.empty()) modelin dokusu yok.
struct TextureData_t {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> pixels;
};

struct SkinnedModel_t {
    std::string name;
    TextureData_t baseColor;
    std::vector<SkinnedVertex_t> vertices;
    std::vector<uint32_t> indices;
    std::vector<Node_t> nodes;
    std::vector<Joint_t> joints;
    std::vector<Clip_t> clips;
};

// Bir klibin bir andaki hali. clip < 0: bind pozu.
struct ClipSample_t {
    int clip = -1;
    float seconds = 0.0f;  // klip süresine göre sarılır (döngü)
};

// Klip adının indeksi; yoksa -1.
[[nodiscard]] ENGINE_API int findClip(const SkinnedModel_t& model, std::string_view name);

// `from` ile `to` pozunu yerel (TRS) düzeyde karıştırır (blend: 0 = from, 1 = to), hiyerarşiyi çözer ve
// her eklem için skin matrisini (düğüm dünya * inverseBind) yazar. skin.size() == model.joints.size().
// Matrisler modelin kendi uzayındadır; çağıran model->dünya dönüşümünü ayrıca uygular.
ENGINE_API void evaluate(const SkinnedModel_t& model, const ClipSample_t& from, const ClipSample_t& to, float blend,
                         std::vector<glm::mat4>& skin);

}  // namespace game::anim
