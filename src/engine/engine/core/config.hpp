#pragma once

#include "engine/engine/api.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace game {

// Plain text settings file: each line is "key value". A line starting with '#' is a comment.
// Example:  player.collisions.enabled 1
using ConfigValues_t = std::unordered_map<std::string, std::string>;

namespace config {

// Returns empty if the file does not exist (defaults are used).
[[nodiscard]] ENGINE_API ConfigValues_t load(const std::filesystem::path& path);

[[nodiscard]] ENGINE_API bool getBool(const ConfigValues_t& values, const std::string& key, bool fallback);
// Fallback if the value is not a number.
[[nodiscard]] ENGINE_API int getInt(const ConfigValues_t& values, const std::string& key, int fallback);
[[nodiscard]] ENGINE_API std::string getString(const ConfigValues_t& values, const std::string& key, const std::string& fallback);
// Fallback if the value is not a number.
[[nodiscard]] ENGINE_API float getFloat(const ConfigValues_t& values, const std::string& key, float fallback);

}  // namespace config
}  // namespace game
