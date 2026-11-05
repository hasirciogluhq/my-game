#include "engine/engine/core/engine_log.hpp"
#include "engine/engine/core/launch_options.hpp"
#include "engine/interface/create_interface.hpp"
#include "engine/interface/module_loader.hpp"
#include "server/i_server.hpp"

#include <array>
#include <filesystem>
#include <print>

// Same host for both servers. The test_server target defines GAME_TEST_SERVER=1 (test content, see game/server/test_scene).
#ifdef GAME_TEST_SERVER
constexpr bool kTestServer = true;
constexpr const char* kHostName = "test_server";
#else
constexpr bool kTestServer = false;
constexpr const char* kHostName = "dedicated_server";
#endif

int main(int argc, char** argv) {
    auto options = game::parseLaunchOptions(argc, argv);
    if (!options) {
        std::println(stderr, "{}: {}", kHostName, options.error());
        return 2;
    }
    options->testScene = kTestServer;

    // Engine modules are in out/.../engine; game modules are in the same folder as the exe (out/.../game).
    const std::filesystem::path gameDir = std::filesystem::absolute(argv[0]).parent_path();
    const std::filesystem::path engineDir = gameDir.parent_path() / "engine";

    // Log first: every later module connects to the sink as it is loaded. Default file: out/.../logs/<host>.log
    if (options->log.file.empty()) {
        options->log.file = (gameDir.parent_path() / "logs" / (std::string(kHostName) + ".log")).string();
    }
    game::CModule logModule;
    game::ILog* log = game::startLogging(logModule, engineDir, options->log);
    game::bindEngineLog(log);

    // "server" last: it uses physics and network in connect().
    struct ModuleSpec_t {
        const char* name;
        bool isEngine;
    };
    constexpr std::array kModules{ModuleSpec_t{"physics", true}, ModuleSpec_t{"network", true}, ModuleSpec_t{"server", false}};

    std::array<game::CModule, kModules.size()> modules;
    for (std::size_t i = 0; i < kModules.size(); ++i) {
        const auto& dir = kModules[i].isEngine ? engineDir : gameDir;
        if (!modules[i].load(dir / game::moduleFileName(kModules[i].name))) {
            return 1;
        }
        game::registerModuleFactory(modules[i].factory());
    }

    auto* app = modules.back().getInterface<game::IServerApp>(game::kServerAppInterface);
    if (app == nullptr) {
        GAME_LOG_ERROR("server", "{} interface not found", game::kServerAppInterface);
        return 1;
    }
    if (!app->connect(&game::resolveInterface)) {
        return 1;
    }
    const int result = app->run(*options);
    if (log != nullptr) {
        log->flush();
    }
    return result;
}
