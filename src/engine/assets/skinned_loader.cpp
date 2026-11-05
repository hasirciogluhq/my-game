#include "engine/assets/skinned_loader.hpp"

#include "engine/interface/render_types.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <functional>
#include <span>
#include <unordered_map>

#include <glm/gtc/type_ptr.hpp>

namespace game::model_loader {
namespace {

using NodeIndex_t = std::unordered_map<std::string, int>;

glm::vec3 toGlm(const aiVector3D& v) {
    return {v.x, v.y, v.z};
}

glm::quat toGlm(const aiQuaternion& q) {
    return {q.w, q.x, q.y, q.z};
}

// Assimp matrisi satır-öncelikli, glm sütun-öncelikli.
glm::mat4 toGlm(const aiMatrix4x4& m) {
    return glm::transpose(glm::make_mat4(&m.a1));
}

// Ağacı ebeveyn önce gelecek şekilde düz listeye çevirir.
void flattenNodes(const aiNode* node, int parent, anim::SkinnedModel_t& model, NodeIndex_t& indexOf) {
    aiVector3D scale;
    aiQuaternion rotation;
    aiVector3D translation;
    node->mTransformation.Decompose(scale, rotation, translation);

    const int index = static_cast<int>(model.nodes.size());
    model.nodes.push_back({.name = node->mName.C_Str(),
                           .parent = parent,
                           .translation = toGlm(translation),
                           .rotation = toGlm(rotation),
                           .scale = toGlm(scale)});
    indexOf.emplace(node->mName.C_Str(), index);
    for (const aiNode* child : std::span{node->mChildren, node->mNumChildren}) {
        flattenNodes(child, index, model, indexOf);
    }
}

struct Influence_t {
    uint8_t joint = 0;
    float weight = 0.0f;
};

// Bir mesh'in eklemlerini modele ekler (ad başına bir kez) ve vertex başına etkileri toplar.
std::expected<std::vector<std::vector<Influence_t>>, std::string> collectInfluences(const aiMesh* mesh, const NodeIndex_t& nodeIndex,
                                                                                    anim::SkinnedModel_t& model) {
    std::vector<std::vector<Influence_t>> influences(mesh->mNumVertices);
    for (const aiBone* bone : std::span{mesh->mBones, mesh->mNumBones}) {
        const auto node = nodeIndex.find(bone->mName.C_Str());
        if (node == nodeIndex.end()) {
            return std::unexpected(std::string{"eklem düğümü bulunamadı: "} + bone->mName.C_Str());
        }
        auto joint = std::ranges::find(model.joints, node->second, &anim::Joint_t::node);
        if (joint == model.joints.end()) {
            if (model.joints.size() >= kMaxBones) {
                return std::unexpected("eklem sayısı kMaxBones'u aşıyor (" + std::to_string(kMaxBones) + ")");
            }
            model.joints.push_back({.node = node->second, .inverseBind = toGlm(bone->mOffsetMatrix)});
            joint = model.joints.end() - 1;
        }
        const auto jointIndex = static_cast<uint8_t>(joint - model.joints.begin());
        for (const aiVertexWeight& weight : std::span{bone->mWeights, bone->mNumWeights}) {
            influences[weight.mVertexId].push_back({jointIndex, weight.mWeight});
        }
    }
    return influences;
}

// En güçlü 4 etkiyi alır, toplamı 1 yapar. Etkisiz vertex joint 0'a tam bağlanır.
void writeSkin(anim::SkinnedVertex_t& vertex, std::vector<Influence_t> influences) {
    std::ranges::sort(influences, std::greater<>{}, &Influence_t::weight);
    influences.resize(std::min<size_t>(influences.size(), 4));
    float total = 0.0f;
    for (const Influence_t& influence : influences) {
        total += influence.weight;
    }
    if (total <= 0.0f) {
        vertex.weights = {1.0f, 0.0f, 0.0f, 0.0f};
        return;
    }
    for (size_t i = 0; i < influences.size(); ++i) {
        vertex.joints[i] = influences[i].joint;
        vertex.weights[static_cast<int>(i)] = influences[i].weight / total;
    }
}

std::expected<void, std::string> appendMesh(const aiMesh* mesh, const NodeIndex_t& nodeIndex, anim::SkinnedModel_t& model) {
    auto influences = collectInfluences(mesh, nodeIndex, model);
    if (!influences) {
        return std::unexpected(influences.error());
    }
    const auto base = static_cast<uint32_t>(model.vertices.size());
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        anim::SkinnedVertex_t vertex;
        vertex.position = toGlm(mesh->mVertices[i]);
        if (mesh->HasNormals()) {
            vertex.normal = toGlm(mesh->mNormals[i]);
        }
        if (mesh->HasTextureCoords(0)) {
            vertex.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        }
        writeSkin(vertex, std::move((*influences)[i]));
        model.vertices.push_back(vertex);
    }
    for (const aiFace& face : std::span{mesh->mFaces, mesh->mNumFaces}) {
        for (const unsigned int index : std::span{face.mIndices, face.mNumIndices}) {
            model.indices.push_back(base + index);
        }
    }
    return {};
}

template <typename Out, typename Key, typename Convert>
std::vector<anim::Key_t<Out>> convertKeys(const Key* keys, unsigned int count, double ticksPerSecond, Convert convert) {
    std::vector<anim::Key_t<Out>> out;
    out.reserve(count);
    for (const Key& key : std::span{keys, count}) {
        out.push_back({.time = static_cast<float>(key.mTime / ticksPerSecond), .value = convert(key.mValue)});
    }
    return out;
}

anim::Clip_t convertClip(const aiAnimation* animation, const NodeIndex_t& nodeIndex) {
    const double ticksPerSecond = animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 25.0;
    anim::Clip_t clip{.name = animation->mName.C_Str(), .duration = static_cast<float>(animation->mDuration / ticksPerSecond)};
    for (const aiNodeAnim* channel : std::span{animation->mChannels, animation->mNumChannels}) {
        const auto node = nodeIndex.find(channel->mNodeName.C_Str());
        if (node == nodeIndex.end()) {
            continue;
        }
        clip.channels.push_back(
            {.node = node->second,
             .translations = convertKeys<glm::vec3>(channel->mPositionKeys, channel->mNumPositionKeys, ticksPerSecond, [](const aiVector3D& v) { return toGlm(v); }),
             .rotations = convertKeys<glm::quat>(channel->mRotationKeys, channel->mNumRotationKeys, ticksPerSecond, [](const aiQuaternion& q) { return toGlm(q); }),
             .scales = convertKeys<glm::vec3>(channel->mScalingKeys, channel->mNumScalingKeys, ticksPerSecond, [](const aiVector3D& v) { return toGlm(v); })});
    }
    return clip;
}

}  // namespace

std::expected<anim::SkinnedModel_t, std::string> loadSkinned(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
                                                       aiProcess_LimitBoneWeights);
    if (scene == nullptr || scene->mRootNode == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0) {
        return std::unexpected(importer.GetErrorString());
    }

    anim::SkinnedModel_t model;
    model.name = path;
    NodeIndex_t nodeIndex;
    flattenNodes(scene->mRootNode, -1, model, nodeIndex);

    for (const aiMesh* mesh : std::span{scene->mMeshes, scene->mNumMeshes}) {
        if (!mesh->HasBones()) {
            return std::unexpected("mesh iskeletli değil (kemik yok): " + path);
        }
        if (auto added = appendMesh(mesh, nodeIndex, model); !added) {
            return std::unexpected(added.error());
        }
    }
    for (const aiAnimation* animation : std::span{scene->mAnimations, scene->mNumAnimations}) {
        model.clips.push_back(convertClip(animation, nodeIndex));
    }
    return model;
}

}  // namespace game::model_loader
