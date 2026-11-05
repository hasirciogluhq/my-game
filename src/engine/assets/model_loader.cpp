#include "engine/assets/model_loader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <span>

namespace game::model_loader {

std::expected<MeshData_t, std::string> load(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality);
    if (scene == nullptr || scene->mRootNode == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0) {
        return std::unexpected(importer.GetErrorString());
    }

    MeshData_t mesh;
    mesh.name = path;
    for (const aiMesh* src : std::span{scene->mMeshes, scene->mNumMeshes}) {
        const auto vertexBase = static_cast<uint32_t>(mesh.vertices.size());
        for (unsigned int i = 0; i < src->mNumVertices; ++i) {
            Vertex_t vertex;
            vertex.position = {src->mVertices[i].x, src->mVertices[i].y, src->mVertices[i].z};
            if (src->HasNormals()) {
                vertex.normal = {src->mNormals[i].x, src->mNormals[i].y, src->mNormals[i].z};
            }
            if (src->HasTextureCoords(0)) {
                vertex.uv = {src->mTextureCoords[0][i].x, src->mTextureCoords[0][i].y};
            }
            mesh.vertices.push_back(vertex);
        }
        for (const aiFace& face : std::span{src->mFaces, src->mNumFaces}) {
            for (unsigned int index : std::span{face.mIndices, face.mNumIndices}) {
                mesh.indices.push_back(vertexBase + index);
            }
        }
    }
    return mesh;
}

}  // namespace game::model_loader
