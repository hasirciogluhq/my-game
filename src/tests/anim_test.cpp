// Headless animation test: loads the robot model (robot.glb, Assimp), skin weights, pose calculation, clip blending.
// Run: xmake run anim_test
#include "engine/assets/skinned_loader.hpp"
#include "engine/engine/anim/skinned_model.hpp"
#include "engine/engine/core/asset_path.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

int g_failures = 0;

void check(bool ok, const char* what) {
    std::cout << (ok ? "ok    " : "FAIL  ") << what << "\n";
    g_failures += ok ? 0 : 1;
}

// Transforms a vertex with the skin matrices (same sum as skinMatrix in the shader).
glm::vec3 skinned(const game::anim::SkinnedVertex_t& vertex, const std::vector<glm::mat4>& skin) {
    glm::mat4 matrix{0.0f};
    for (int i = 0; i < 4; ++i) {
        matrix += skin[vertex.joints[static_cast<size_t>(i)]] * vertex.weights[i];
    }
    return glm::vec3{matrix * glm::vec4{vertex.position, 1.0f}};
}

// The highest point of the right hand (x > 0.2, y < 1.0) in the bind pose.
float rightHandHeight(const game::anim::SkinnedModel_t& model, const std::vector<glm::mat4>& skin) {
    float height = -1.0f;
    for (const auto& vertex : model.vertices) {
        if (vertex.position.x > 0.2f && vertex.position.y < 1.0f && vertex.position.y > 0.75f) {
            height = std::max(height, skinned(vertex, skin).y);
        }
    }
    return height;
}

bool nearIdentity(const std::vector<glm::mat4>& skin, float epsilon) {
    return std::ranges::all_of(skin, [epsilon](const glm::mat4& m) {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                if (std::abs(m[c][r] - (c == r ? 1.0f : 0.0f)) > epsilon) {
                    return false;
                }
            }
        }
        return true;
    });
}

}  // namespace

int main() {
    using namespace game;
    const auto file = findAssetPath("models/robot.glb");
    check(file.has_value(), "models/robot.glb found");
    if (!file) {
        return 1;
    }
    auto loaded = model_loader::loadSkinned(file->string());
    check(loaded.has_value(), "skinned model loaded");
    if (!loaded) {
        std::cout << loaded.error() << "\n";
        return 1;
    }
    const anim::SkinnedModel_t& model = *loaded;
    std::cout << model.vertices.size() << " vertices, " << model.indices.size() / 3 << " triangles, " << model.joints.size() << " joints, "
              << model.nodes.size() << " nodes, " << model.clips.size() << " clips\n";

    check(model.joints.size() == 11, "11 joints");
    const int idle = anim::findClip(model, "idle");
    const int wave = anim::findClip(model, "wave");
    check(idle >= 0 && wave >= 0, "idle and wave clips exist");
    check(anim::findClip(model, "yok") == -1, "missing clip gives -1");
    check(std::abs(model.clips[static_cast<size_t>(wave)].duration - 1.6f) < 1e-3f, "wave duration is 1.6 s");

    const bool weightsOk = std::ranges::all_of(model.vertices, [](const anim::SkinnedVertex_t& v) {
        return std::abs(v.weights.x + v.weights.y + v.weights.z + v.weights.w - 1.0f) < 1e-4f;
    });
    check(weightsOk, "every vertex's skin weights sum to 1");

    std::vector<glm::mat4> skin;
    anim::evaluate(model, {}, {}, 1.0f, skin);
    check(skin.size() == model.joints.size(), "number of skin matrices = number of joints");
    check(nearIdentity(skin, 1e-4f), "bind pose: all skin matrices are identity");
    const float restHand = rightHandHeight(model, skin);

    anim::evaluate(model, {}, {.clip = idle, .seconds = 0.0f}, 1.0f, skin);
    check(nearIdentity(skin, 0.1f) && !nearIdentity(skin, 1e-3f), "idle t=0 is close to the bind pose, but the arms are slightly open");

    anim::evaluate(model, {}, {.clip = wave, .seconds = 0.4f}, 1.0f, skin);
    const float waveHand = rightHandHeight(model, skin);
    check(waveHand > 1.4f && restHand < 1.0f, "wave: the right hand rose above the shoulder");

    anim::evaluate(model, {.clip = idle, .seconds = 0.4f}, {.clip = wave, .seconds = 0.4f}, 0.5f, skin);
    const float halfHand = rightHandHeight(model, skin);
    check(halfHand > restHand + 0.05f && halfHand < waveHand, "blend 0.5: between idle and wave");

    anim::evaluate(model, {}, {.clip = wave, .seconds = 0.4f + 1.6f}, 1.0f, skin);
    check(std::abs(rightHandHeight(model, skin) - waveHand) < 1e-3f, "loops after the clip duration");

    std::cout << (g_failures == 0 ? "ALL TESTS PASSED" : "ERROR") << "\n";
    return g_failures == 0 ? 0 : 1;
}
