#include "engine/engine/core/primitives.hpp"

#include <utility>

namespace game {
namespace {

// Adds one face. u and v must be chosen so that u x v = normal (CCW seen from outside).
void addFace(MeshData_t& mesh, glm::vec3 normal, glm::vec3 u, glm::vec3 v) {
    const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
    const glm::vec3 center = normal * 0.5f;

    mesh.vertices.push_back(Vertex_t{.position = center - 0.5f * u - 0.5f * v, .normal = normal, .uv = {0.0f, 0.0f}});
    mesh.vertices.push_back(Vertex_t{.position = center + 0.5f * u - 0.5f * v, .normal = normal, .uv = {1.0f, 0.0f}});
    mesh.vertices.push_back(Vertex_t{.position = center + 0.5f * u + 0.5f * v, .normal = normal, .uv = {1.0f, 1.0f}});
    mesh.vertices.push_back(Vertex_t{.position = center - 0.5f * u + 0.5f * v, .normal = normal, .uv = {0.0f, 1.0f}});

    mesh.indices.insert(mesh.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

}  // namespace

MeshData_t makeBoxMesh(std::string name) {
    MeshData_t mesh;
    mesh.name = std::move(name);
    addFace(mesh, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
    addFace(mesh, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f});
    addFace(mesh, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f});
    addFace(mesh, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
    addFace(mesh, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    addFace(mesh, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
    return mesh;
}

MeshData_t makePlaneMesh(std::string name, float halfExtent) {
    MeshData_t mesh;
    mesh.name = std::move(name);
    const glm::vec3 normal{0.0f, 1.0f, 0.0f};
    const float h = halfExtent;
    // Indices wind CCW when viewed from +Y (front face up).
    mesh.vertices = {
        Vertex_t{.position = {-h, 0.0f, -h}, .normal = normal, .uv = {0.0f, 0.0f}},
        Vertex_t{.position = {h, 0.0f, -h}, .normal = normal, .uv = {1.0f, 0.0f}},
        Vertex_t{.position = {h, 0.0f, h}, .normal = normal, .uv = {1.0f, 1.0f}},
        Vertex_t{.position = {-h, 0.0f, h}, .normal = normal, .uv = {0.0f, 1.0f}},
    };
    mesh.indices = {0, 2, 1, 0, 3, 2};
    return mesh;
}

}  // namespace game
