#pragma once

#include "engine/engine/api.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace game {

// Düz metin ayar dosyası: her satır "anahtar değer". '#' ile başlayan satır yorumdur.
// Örnek:  player.collisions.enabled 1
using ConfigValues_t = std::unordered_map<std::string, std::string>;

namespace config {

// Dosya yoksa boş döner (varsayılanlar kullanılır).
[[nodiscard]] ENGINE_API ConfigValues_t load(const std::filesystem::path& path);

[[nodiscard]] ENGINE_API bool getBool(const ConfigValues_t& values, const std::string& key, bool fallback);
// Sayı değilse fallback.
[[nodiscard]] ENGINE_API int getInt(const ConfigValues_t& values, const std::string& key, int fallback);

}  // namespace config
}  // namespace game
