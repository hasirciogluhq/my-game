#pragma once

#include "engine/engine/api.hpp"
#include "engine/engine/core/types.hpp"
#include "engine/interface/i_log.hpp"

#include <cstdint>
#include <expected>
#include <string>

namespace game {

// Command line: --tick-rate=64 --host=127.0.0.1 --port=7777
//               --log-level=debug --log-cat=net:trace,render:warn --log-file=path.log
struct LaunchOptions_t {
    uint32_t tickRate = kDefaultTickRate;
    std::string host = "127.0.0.1";
    uint16_t port = kDefaultPort;
    LogConfig_t log;
    // Set by the test_game host (not a command line flag): the map spawns test content (game/server/test_scene).
    bool testScene = false;
};

// argv: char** (argv[0] is the program name and is skipped).
[[nodiscard]] ENGINE_API std::expected<LaunchOptions_t, std::string> parseLaunchOptions(int argc, char** argv);

}  // namespace game
