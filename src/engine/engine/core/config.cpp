#include "engine/engine/core/config.hpp"

#include <charconv>
#include <fstream>
#include <sstream>

namespace game::config {

ConfigValues_t load(const std::filesystem::path& path) {
    ConfigValues_t values;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream words(line);
        std::string key;
        std::string value;
        if (!(words >> key >> value) || key.starts_with('#')) {
            continue;
        }
        values[key] = value;
    }
    return values;
}

bool getBool(const ConfigValues_t& values, const std::string& key, bool fallback) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return fallback;
    }
    return it->second == "1" || it->second == "true";
}

int getInt(const ConfigValues_t& values, const std::string& key, int fallback) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return fallback;
    }
    int number = fallback;
    const auto& text = it->second;
    std::from_chars(text.data(), text.data() + text.size(), number);  // on error, number keeps the fallback
    return number;
}

}  // namespace game::config
