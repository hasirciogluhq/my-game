#include "engine/engine/core/launch_options.hpp"

#include <charconv>
#include <string_view>

namespace game {
namespace {

template <typename Integer>
std::expected<Integer, std::string> parseInteger(std::string_view text, std::string_view flag) {
    Integer value{};
    const char* first = text.data();
    const char* last = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last) {
        return std::unexpected(std::string(flag) + " sayı olmalı: '" + std::string(text) + "'");
    }
    return value;
}

}  // namespace

std::expected<LaunchOptions_t, std::string> parseLaunchOptions(int argc, char** argv) {
    LaunchOptions_t options;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if (arg.starts_with("--tick-rate=")) {
            auto rate = parseInteger<uint32_t>(arg.substr(12), "--tick-rate");
            if (!rate) {
                return std::unexpected(rate.error());
            }
            if (!isValidTickRate(*rate)) {
                return std::unexpected("--tick-rate " + std::to_string(kMinTickRate) + "-" + std::to_string(kMaxTickRate) + " aralığında olmalı");
            }
            options.tickRate = *rate;
        } else if (arg.starts_with("--host=")) {
            options.host = std::string(arg.substr(7));
        } else if (arg.starts_with("--port=")) {
            auto port = parseInteger<uint16_t>(arg.substr(7), "--port");
            if (!port) {
                return std::unexpected(port.error());
            }
            options.port = *port;
        } else if (arg.starts_with("--log-level=")) {
            const auto level = parseLogLevel(arg.substr(12));
            if (!level) {
                return std::unexpected("--log-level: trace|debug|info|warn|error|fatal|off olmalı");
            }
            options.log.level = *level;
        } else if (arg.starts_with("--log-cat=")) {
            options.log.categoryLevels = std::string(arg.substr(10));
        } else if (arg.starts_with("--log-file=")) {
            options.log.file = std::string(arg.substr(11));
        } else {
            return std::unexpected("bilinmeyen argüman: " + std::string(arg));
        }
    }
    return options;
}

}  // namespace game
