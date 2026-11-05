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

// Skeleton + skin + animation clip: only data and pose math. Which clip plays when is a GAME decision
// (game/server/systems/animation); the engine only produces the pose for "this clip, at this second".
namespace game::anim {

// The joint count cannot exceed the renderer's kMaxBones (render_types.hpp); the loader rejects models that do.
// The vertex sent to the GPU has EXACTLY the same layout as render_types.hpp EVertexLayout::kPosNormalUvSkin (52 bytes).
struct SkinnedVertex_t {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
    std::array<uint8_t, 4> joints{};  // joint index (SkinnedModel_t::joints)
    glm::vec4 weights{0.0f};          // sums to 1
};
static_assert(sizeof(SkinnedVertex_t) == 52);

// A node in the scene tree. The nodes array is ordered so that parents come before their children.
struct Node_t {
    std::string name;
    int parent = -1;
    glm::vec3 translation{0.0f};  // local transform in the bind pose
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

// The joint a vertex's joint index points to: the node, plus the matrix that undoes the bind pose.
struct Joint_t {
    int node = 0;
    glm::mat4 inverseBind{1.0f};
};

template <typename T>
struct Key_t {
    float time = 0.0f;  // seconds
    T value{};
};

// One node's animation. An empty track means the bind pose is used.
struct Channel_t {
    int node = 0;
    std::vector<Key_t<glm::vec3>> translations;
    std::vector<Key_t<glm::quat>> rotations;
    std::vector<Key_t<glm::vec3>> scales;
};

struct Clip_t {
    std::string name;
    float duration = 0.0f;  // seconds; clips loop
    std::vector<Channel_t> channels;
};

// Decoded (RGBA8, sRGB color) base color texture. If empty (pixels.empty()), the model has no texture.
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

// State of one clip at a given moment. clip < 0: bind pose.
struct ClipSample_t {
    int clip = -1;
    float seconds = 0.0f;  // wrapped according to the clip duration (loop)
};

// Index of the clip by name; -1 if it does not exist.
[[nodiscard]] ENGINE_API int findClip(const SkinnedModel_t& model, std::string_view name);

// Blends the `from` and `to` poses at the local (TRS) level (blend: 0 = from, 1 = to), resolves the hierarchy, and
// writes the skin matrix (node world * inverseBind) for each joint. skin.size() == model.joints.size().
// Matrices are in the model's own space; the caller applies the model->world transform separately.
ENGINE_API void evaluate(const SkinnedModel_t& model, const ClipSample_t& from, const ClipSample_t& to, float blend,
                         std::vector<glm::mat4>& skin);

}  // namespace game::anim
