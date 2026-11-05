#include "engine/engine/anim/skinned_model.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace game::anim {
namespace {

struct Local_t {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

// Value at time `time` in the key array. A single key gives a constant value. lerp: (a, b, t) -> value.
template <typename T, typename Lerp>
T sampleKeys(const std::vector<Key_t<T>>& keys, float time, Lerp lerp) {
    if (keys.size() == 1 || time <= keys.front().time) {
        return keys.front().value;
    }
    if (time >= keys.back().time) {
        return keys.back().value;
    }
    const auto next = std::ranges::upper_bound(keys, time, {}, &Key_t<T>::time);
    const auto previous = next - 1;
    const float t = (time - previous->time) / (next->time - previous->time);
    return lerp(previous->value, next->value, t);
}

// The clip's channel for each node (nullptr if none). clip < 0: all nullptr (bind pose).
std::vector<const Channel_t*> channelsByNode(const SkinnedModel_t& model, int clip) {
    std::vector<const Channel_t*> byNode(model.nodes.size(), nullptr);
    if (clip >= 0 && clip < static_cast<int>(model.clips.size())) {
        for (const Channel_t& channel : model.clips[static_cast<size_t>(clip)].channels) {
            byNode[static_cast<size_t>(channel.node)] = &channel;
        }
    }
    return byNode;
}

float wrappedTime(const SkinnedModel_t& model, const ClipSample_t& sample) {
    if (sample.clip < 0 || sample.clip >= static_cast<int>(model.clips.size())) {
        return 0.0f;
    }
    const float duration = model.clips[static_cast<size_t>(sample.clip)].duration;
    return duration > 0.0f ? std::fmod(std::max(sample.seconds, 0.0f), duration) : 0.0f;
}

Local_t sampleNode(const Node_t& node, const Channel_t* channel, float time) {
    Local_t local{.translation = node.translation, .rotation = node.rotation, .scale = node.scale};
    if (channel == nullptr) {
        return local;
    }
    if (!channel->translations.empty()) {
        local.translation = sampleKeys(channel->translations, time, [](const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); });
    }
    if (!channel->rotations.empty()) {
        local.rotation = sampleKeys(channel->rotations, time, [](const glm::quat& a, const glm::quat& b, float t) { return glm::slerp(a, b, t); });
    }
    if (!channel->scales.empty()) {
        local.scale = sampleKeys(channel->scales, time, [](const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); });
    }
    return local;
}

glm::mat4 toMatrix(const Local_t& local) {
    return glm::translate(glm::mat4{1.0f}, local.translation) * glm::mat4_cast(local.rotation) * glm::scale(glm::mat4{1.0f}, local.scale);
}

}  // namespace

int findClip(const SkinnedModel_t& model, std::string_view name) {
    const auto found = std::ranges::find(model.clips, name, &Clip_t::name);
    return found != model.clips.end() ? static_cast<int>(found - model.clips.begin()) : -1;
}

void evaluate(const SkinnedModel_t& model, const ClipSample_t& from, const ClipSample_t& to, float blend, std::vector<glm::mat4>& skin) {
    const std::vector<const Channel_t*> fromChannels = channelsByNode(model, from.clip);
    const std::vector<const Channel_t*> toChannels = channelsByNode(model, to.clip);
    const float fromTime = wrappedTime(model, from);
    const float toTime = wrappedTime(model, to);
    const float weight = std::clamp(blend, 0.0f, 1.0f);

    // nodes are sorted parent-first: world matrices are computed in a single pass.
    std::vector<glm::mat4> world(model.nodes.size());
    for (size_t i = 0; i < model.nodes.size(); ++i) {
        const Node_t& node = model.nodes[i];
        Local_t local = sampleNode(node, toChannels[i], toTime);
        if (weight < 1.0f) {
            const Local_t before = sampleNode(node, fromChannels[i], fromTime);
            local = {.translation = glm::mix(before.translation, local.translation, weight),
                     .rotation = glm::slerp(before.rotation, local.rotation, weight),
                     .scale = glm::mix(before.scale, local.scale, weight)};
        }
        const glm::mat4 matrix = toMatrix(local);
        world[i] = node.parent < 0 ? matrix : world[static_cast<size_t>(node.parent)] * matrix;
    }

    skin.resize(model.joints.size());
    for (size_t i = 0; i < model.joints.size(); ++i) {
        skin[i] = world[static_cast<size_t>(model.joints[i].node)] * model.joints[i].inverseBind;
    }
}

}  // namespace game::anim
