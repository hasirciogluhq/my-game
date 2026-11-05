#pragma once

#include "engine/engine/api.hpp"

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace game::gzip {

[[nodiscard]] ENGINE_API std::expected<std::vector<uint8_t>, std::string> compress(std::span<const uint8_t> input);
[[nodiscard]] ENGINE_API std::expected<std::vector<uint8_t>, std::string> decompress(std::span<const uint8_t> input);

}  // namespace game::gzip
