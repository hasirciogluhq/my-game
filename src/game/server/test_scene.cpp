#include "game/server/test_scene.hpp"

#include <array>
#include <random>

namespace game::testscene {
namespace {

constexpr int kRandomPropCount = 8;
constexpr float kAreaHalfSize = 15.0f;  // metres: props are placed in a square around the spawn
constexpr std::array kModels{EModel::kRobot, EModel::kDancer};

}  // namespace

void spawnRandomProps(World_t& world) {
    std::random_device device;
    std::mt19937 random{device()};
    std::uniform_real_distribution<float> offset{-kAreaHalfSize, kAreaHalfSize};
    std::uniform_real_distribution<float> angle{0.0f, 6.2831853f};
    std::uniform_int_distribution<std::size_t> pick{0, kModels.size() - 1};

    for (int i = 0; i < kRandomPropCount; ++i) {
        const glm::vec3 position{offset(random), 0.0f, offset(random)};
        std::ignore = world::spawnAnimated(world, kModels[pick(random)], position, angle(random));
    }
}

}  // namespace game::testscene
