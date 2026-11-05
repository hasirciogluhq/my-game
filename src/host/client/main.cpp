#include "client/i_client.hpp"
#include "engine/engine/core/engine_log.hpp"
#include "engine/engine/core/launch_options.hpp"
#include "engine/interface/create_interface.hpp"
#include "engine/interface/module_loader.hpp"

#include <array>
#include <filesystem>
#include <print>

int main(int argc, char** argv) {
    auto options = game::parseLaunchOptions(argc, argv);
    if (!options) {
        std::println(stderr, "game: {}", options.error());
        return 2;
    }

    // Motor modülleri out/.../engine, oyun modülleri exe ile aynı klasörde (out/.../game).
    const std::filesystem::path gameDir = std::filesystem::absolute(argv[0]).parent_path();
    const std::filesystem::path engineDir = gameDir.parent_path() / "engine";

    // Önce log: sonraki tüm modüller yüklenirken sink'e bağlanır. Varsayılan dosya: out/.../logs/game.log
    if (options->log.file.empty()) {
        options->log.file = (gameDir.parent_path() / "logs" / "game.log").string();
    }
    game::CModule logModule;
    game::ILog* log = game::startLogging(logModule, engineDir, options->log);
    game::bindEngineLog(log);

    // "client" son: diğerlerini connect'te kullanır.
    struct ModuleSpec_t {
        const char* name;
        bool isEngine;
    };
    constexpr std::array kModules{ModuleSpec_t{"platform", true}, ModuleSpec_t{"render", true},
                                  ModuleSpec_t{"particles", true}, ModuleSpec_t{"assets", true},
                                  ModuleSpec_t{"audio", true},     ModuleSpec_t{"ui", true},
                                  ModuleSpec_t{"physics", true},  // client prediction için
                                  ModuleSpec_t{"network", true},  ModuleSpec_t{"client", false}};

    std::array<game::CModule, kModules.size()> modules;
    for (std::size_t i = 0; i < kModules.size(); ++i) {
        const auto& dir = kModules[i].isEngine ? engineDir : gameDir;
        if (!modules[i].load(dir / game::moduleFileName(kModules[i].name))) {
            return 1;
        }
        game::registerModuleFactory(modules[i].factory());
    }

    auto* app = modules.back().getInterface<game::IClientApp>(game::kClientAppInterface);
    if (app == nullptr) {
        GAME_LOG_ERROR("game", "{} arayüzü bulunamadı", game::kClientAppInterface);
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
