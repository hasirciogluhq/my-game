#include "engine/assets/skinned_loader.hpp"

#include "engine/interface/render_types.hpp"

#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
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

// Assimp matrices are row-major, glm matrices are column-major.
glm::mat4 toGlm(const aiMatrix4x4& m) {
    return glm::transpose(glm::make_mat4(&m.a1));
}

// Flattens the tree into a list with parents before their children.
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

// Adds a mesh's joints to the model (once per name) and collects the influences per vertex.
std::expected<std::vector<std::vector<Influence_t>>, std::string> collectInfluences(const aiMesh* mesh, const NodeIndex_t& nodeIndex,
                                                                                    anim::SkinnedModel_t& model) {
    std::vector<std::vector<Influence_t>> influences(mesh->mNumVertices);
    for (const aiBone* bone : std::span{mesh->mBones, mesh->mNumBones}) {
        const auto node = nodeIndex.find(bone->mName.C_Str());
        if (node == nodeIndex.end()) {
            return std::unexpected(std::string{"joint node not found: "} + bone->mName.C_Str());
        }
        auto joint = std::ranges::find(model.joints, node->second, &anim::Joint_t::node);
        if (joint == model.joints.end()) {
            if (model.joints.size() >= kMaxBones) {
                return std::unexpected("joint count exceeds kMaxBones (" + std::to_string(kMaxBones) + ")");
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

// Keeps the 4 strongest influences and normalizes them to sum to 1. An unweighted vertex is fully bound to joint 0.
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
            vertex.uv = {mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y};  // Assimp V points up, Metal V points down
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

// Decodes to RGBA8; an empty texture on failure.
anim::TextureData_t decodeImage(const unsigned char* bytes, int size) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(bytes, size, &width, &height, &channels, 4);
    if (pixels == nullptr) {
        return {};
    }
    anim::TextureData_t texture{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)};
    texture.pixels.assign(pixels, pixels + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    stbi_image_free(pixels);
    return texture;
}

std::vector<unsigned char> readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// Base color texture of the first mesh: embedded in the glb, or a file next to the model.
anim::TextureData_t loadBaseColor(const aiScene* scene, const std::filesystem::path& modelPath) {
    for (const aiMesh* mesh : std::span{scene->mMeshes, scene->mNumMeshes}) {
        aiString texturePath;
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) != AI_SUCCESS &&
            material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) != AI_SUCCESS) {
            continue;
        }
        if (const aiTexture* embedded = scene->GetEmbeddedTexture(texturePath.C_Str()); embedded != nullptr && embedded->mHeight == 0) {
            return decodeImage(reinterpret_cast<const unsigned char*>(embedded->pcData), static_cast<int>(embedded->mWidth));
        }
        const auto bytes = readFile(modelPath.parent_path() / std::filesystem::path{texturePath.C_Str()}.filename());
        return decodeImage(bytes.data(), static_cast<int>(bytes.size()));
    }
    return {};
}

std::unique_ptr<Assimp::Importer> makeImporter() {
    auto importer = std::make_unique<Assimp::Importer>();
    importer->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);  // Mixamo: resolve pivot nodes
    return importer;
}

constexpr unsigned kImportFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_LimitBoneWeights;

bool isModelFile(const std::filesystem::path& path) {
    const auto ext = path.extension().string();
    return ext == ".glb";
}

// Directory mode: character.* = mesh + skeleton, other model files = extra clips (clip name = file name).
std::expected<anim::SkinnedModel_t, std::string> loadDirectory(const std::filesystem::path& dir) {
    std::filesystem::path character;
    std::vector<std::filesystem::path> clipFiles;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file() || !isModelFile(entry.path())) {
            continue;
        }
        if (entry.path().stem() == "character") {
            character = entry.path();
        } else {
            clipFiles.push_back(entry.path());
        }
    }
    if (character.empty()) {
        return std::unexpected("no character.fbx/glb in folder: " + dir.string());
    }
    std::ranges::sort(clipFiles);

    auto model = loadSkinned(character.string());
    if (!model) {
        return model;
    }
    NodeIndex_t nodeIndex;
    for (size_t i = 0; i < model->nodes.size(); ++i) {
        nodeIndex.emplace(model->nodes[i].name, static_cast<int>(i));
    }
    for (const auto& file : clipFiles) {
        const auto importer = makeImporter();
        const aiScene* scene = importer->ReadFile(file.string(), kImportFlags);
        if (scene == nullptr || scene->mNumAnimations == 0) {
            return std::unexpected("could not read clip file: " + file.string());
        }
        anim::Clip_t clip = convertClip(scene->mAnimations[0], nodeIndex);
        clip.name = file.stem().string();
        model->clips.push_back(std::move(clip));
    }
    return model;
}

}  // namespace

std::expected<anim::SkinnedModel_t, std::string> loadSkinned(const std::string& path) {
    if (std::filesystem::is_directory(path)) {
        return loadDirectory(path);
    }
    const auto importer = makeImporter();
    const aiScene* scene = importer->ReadFile(path, kImportFlags);
    if (scene == nullptr || scene->mRootNode == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0) {
        return std::unexpected(importer->GetErrorString());
    }

    anim::SkinnedModel_t model;
    model.name = path;
    NodeIndex_t nodeIndex;
    flattenNodes(scene->mRootNode, -1, model, nodeIndex);

    for (const aiMesh* mesh : std::span{scene->mMeshes, scene->mNumMeshes}) {
        if (!mesh->HasBones()) {
            return std::unexpected("mesh is not skinned (no bones): " + path);
        }
        if (auto added = appendMesh(mesh, nodeIndex, model); !added) {
            return std::unexpected(added.error());
        }
    }
    for (const aiAnimation* animation : std::span{scene->mAnimations, scene->mNumAnimations}) {
        model.clips.push_back(convertClip(animation, nodeIndex));
    }
    model.baseColor = loadBaseColor(scene, path);
    return model;
}

}  // namespace game::model_loader
