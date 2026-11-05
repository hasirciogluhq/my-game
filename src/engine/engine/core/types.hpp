#pragma once

#include "engine/engine/api.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace game {

// ---------------------------------------------------------------------------
// Tick settings. Default 64; can be changed with --tick-rate=N (server).
// The server is authoritative: the client learns the tick rate from the welcome packet.
// ---------------------------------------------------------------------------
inline constexpr uint32_t kDefaultTickRate = 64;
inline constexpr uint32_t kMinTickRate = 10;
inline constexpr uint32_t kMaxTickRate = 512;
inline constexpr uint16_t kDefaultPort = 7777;

[[nodiscard]] constexpr bool isValidTickRate(uint32_t rate) {
    return rate >= kMinTickRate && rate <= kMaxTickRate;
}

[[nodiscard]] constexpr float tickDeltaFor(uint32_t rate) {
    return 1.0f / static_cast<float>(rate);
}

// ---------------------------------------------------------------------------
// Mesh data for the render/asset side
// ---------------------------------------------------------------------------
struct Vertex_t {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
};

struct MeshData_t {
    std::string name;
    std::vector<Vertex_t> vertices;
    std::vector<uint32_t> indices;
};

}  // namespace game
