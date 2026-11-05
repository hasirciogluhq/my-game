#include "client/i_client.hpp"
#include "engine/engine/core/engine_log.hpp"
#include "engine/engine/core/launch_options.hpp"
#include "engine/interface/create_interface.hpp"
#include "engine/interface/module_loader.hpp"
#include "server/i_server.hpp"

#include <array>
#include <filesystem>
#include <print>
#include <thread>

// test_game: the game with a local server built in. server.dll runs in this process on its own thread (test content on),
// and the client joins it through the normal network path (INetwork over TCP+UDP, 127.0.0.1).
int main(int argc, char** argv) {
    auto options = game::parseLaunchOptions(argc, argv);
    if (!options) {
        std::println(stderr, "test_game: {}", options.error());
        return 2;
    }
    options->testScene = true;

    const std::filesystem::path gameDir = std::filesystem::absolute(argv[0]).parent_path();
    const std::filesystem::path engineDir = gameDir.parent_path() / "engine";

    if (options->log.file.empty()) {
        options->log.file = (gameDir.parent_path() / "logs" / "test_game.log").string();
    }
    game::CModule logModule;
    game::ILog* log = game::startLogging(logModule, engineDir, options->log);
    game::bindEngineLog(log);

    // "server" and "client" last: they use the engine modules in connect(). Both containers get their own
    // physics and network instances from the factories.
    struct ModuleSpec_t {
        const char* name;
        bool isEngine;
    };
    constexpr std::array kModules{ModuleSpec_t{"platform", true}, ModuleSpec_t{"render", true},
                                  ModuleSpec_t{"particles", true}, ModuleSpec_t{"assets", true},
                                  ModuleSpec_t{"audio", true},     ModuleSpec_t{"ui", true},
                                  ModuleSpec_t{"physics", true},   ModuleSpec_t{"network", true},
                                  ModuleSpec_t{"server", false},   ModuleSpec_t{"client", false}};

    std::array<game::CModule, kModules.size()> modules;
    for (std::size_t i = 0; i < kModules.size(); ++i) {
        const auto& dir = kModules[i].isEngine ? engineDir : gameDir;
        if (!modules[i].load(dir / game::moduleFileName(kModules[i].name))) {
            return 1;
        }
        game::registerModuleFactory(modules[i].factory());
    }

    auto* server = modules[kModules.size() - 2].getInterface<game::IServerApp>(game::kServerAppInterface);
    auto* client = modules.back().getInterface<game::IClientApp>(game::kClientAppInterface);
    if (server == nullptr || client == nullptr) {
        GAME_LOG_ERROR("game", "server or client interface not found");
        game::exitProcess(log, 1);
    }

    // The local server opens (listens) first, so the client's connect finds it.
    if (!server->connect(&game::resolveInterface) || !server->open(*options)) {
        game::exitProcess(log, 1);
    }
    std::thread([server] { server->loop(); }).detach();  // ends with the process (exitProcess)

    if (!client->connect(&game::resolveInterface)) {
        game::exitProcess(log, 1);
    }
    game::exitProcess(log, client->run(*options));
}
