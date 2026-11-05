#include "client/console/console_commands.hpp"

#include <exception>
#include <format>
#include <sstream>
#include <string>

namespace game::console {
using namespace game::ui;
namespace {

void printHelp(Console_t& console) {
    print(console, "help                   bu liste");
    print(console, "clear                  konsolu temizle");
    print(console, "echo <metin>           metni yazdir");
    print(console, "binds                  tus atamalarini listele");
    print(console, "bind <aksiyon> <tus>   aksiyonu tusa bagla (kaydedilir)");
    print(console, "sensitivity <deger>    fare hassasiyeti (radyan/piksel)");
    print(console, "cl_predict <0|1>       client-side prediction ac/kapa");
    print(console, "cl_interpolate <0|1>   interpolation ac/kapa");
    print(console, "cl_interp <saniye>     interpolation gecikmesi (0.0-0.5)");
    print(console, "pawns                  karakterleri listele (netId, kim suruyor)");
    print(console, "spectate <netId>       karakteri izle (kendi karakterini birakir)");
    print(console, "takeover               izledigin bot'u devral");
    print(console, "release                izlemeyi / bot'u birak, kendi karakterine don");
    print(console, "quit                   oyundan cik");
}

void printBinds(const Context_t& context) {
    for (std::size_t i = 0; i < kActionCount; ++i) {
        const auto action = static_cast<EAction>(i);
        print(context.console, std::format("  {:<10} {}", input::actionName(action), context.bindings[i]));
    }
}

void runBind(Context_t& context, std::istringstream& words) {
    std::string actionText;
    std::string key;
    words >> actionText;
    std::getline(words >> std::ws, key);
    if (actionText.empty() || key.empty()) {
        print(context.console, "kullanim: bind <aksiyon> <tus>   ('binds' ile aksiyonlari gor)", ELine::kError);
        return;
    }
    const auto action = input::actionFromName(actionText);
    if (!action) {
        print(context.console, "bilinmeyen aksiyon: " + actionText, ELine::kError);
        return;
    }
    input::bind(context.bindings, *action, key);
    input::save(context.bindings, input::configPath());
    print(context.console, std::format("{} -> {}", actionText, key));
}

void runSensitivity(Context_t& context, std::istringstream& words) {
    float value = 0.0f;
    if (!(words >> value) || value <= 0.0f) {
        print(context.console, "kullanim: sensitivity <pozitif sayi>   (varsayilan 0.0025)", ELine::kError);
        return;
    }
    context.input.setMouseSensitivity(value);
    print(context.console, std::format("sensitivity = {}", value));
}

void runPredict(Context_t& context, std::istringstream& words) {
    int value = 0;
    if (!(words >> value) || (value != 0 && value != 1)) {
        print(context.console, std::format("kullanim: cl_predict <0|1>   (su an {})", context.prediction.enabled ? 1 : 0), ELine::kError);
        return;
    }
    context.prediction.enabled = value == 1;
    prediction::clear(context.prediction);  // eski tahmin geçmişi geçersiz
    print(context.console, std::format("cl_predict = {}", value));
}

void runInterpolate(Context_t& context, std::istringstream& words) {
    int value = 0;
    if (!(words >> value) || (value != 0 && value != 1)) {
        print(context.console, std::format("kullanim: cl_interpolate <0|1>   (su an {})", context.interpolation.enabled ? 1 : 0), ELine::kError);
        return;
    }
    context.interpolation.enabled = value == 1;
    print(context.console, std::format("cl_interpolate = {}", value));
}

void runInterp(Context_t& context, std::istringstream& words) {
    float value = 0.0f;
    if (!(words >> value) || value < 0.0f || value > 0.5f) {
        print(context.console, std::format("kullanim: cl_interp <0.0-0.5>   (su an {})", context.interpolation.delaySeconds), ELine::kError);
        return;
    }
    context.interpolation.delaySeconds = value;
    print(context.console, std::format("cl_interp = {}", value));
}

void runPawns(const Context_t& context) {
    const uint32_t me = context.network.playerId().value_or(0);
    for (const auto [entity, player, replicated] : context.world.registry.view<const Player_t, const Replicated_t>().each()) {
        const std::string driver = player.playerId == 0 ? "bot/bos" : (player.playerId == me ? "sen" : std::format("oyuncu {}", player.playerId));
        print(context.console, std::format("  netId {:<4} {}", replicated.netId, driver));
    }
}

void runSpectate(Context_t& context, std::istringstream& words) {
    uint32_t netId = 0;
    if (!(words >> netId) || netId == 0) {
        print(context.console, "kullanim: spectate <netId>   ('pawns' ile netId'leri gor)", ELine::kError);
        return;
    }
    context.network.sendControl({.action = EControlAction::kSpectate, .targetNetId = netId});
    print(context.console, std::format("izleme istendi: {}", netId));
}

void runControl(Context_t& context, EControlAction action, const char* text) {
    context.network.sendControl({.action = action});
    print(context.console, text);
}

}  // namespace

void execute(Context_t& context, std::string_view line) {
    std::istringstream words{std::string(line)};
    std::string command;
    words >> command;

    if (command == "help") {
        printHelp(context.console);
    } else if (command == "clear") {
        clear(context.console);
    } else if (command == "echo") {
        std::string rest;
        std::getline(words >> std::ws, rest);
        print(context.console, rest);
    } else if (command == "binds") {
        printBinds(context);
    } else if (command == "bind") {
        runBind(context, words);
    } else if (command == "sensitivity") {
        runSensitivity(context, words);
    } else if (command == "cl_predict") {
        runPredict(context, words);
    } else if (command == "cl_interpolate") {
        runInterpolate(context, words);
    } else if (command == "cl_interp") {
        runInterp(context, words);
    } else if (command == "pawns") {
        runPawns(context);
    } else if (command == "spectate") {
        runSpectate(context, words);
    } else if (command == "takeover") {
        runControl(context, EControlAction::kTakeOver, "devralma istendi (sadece izlenen bot icin)");
    } else if (command == "release") {
        runControl(context, EControlAction::kRelease, "birakma istendi");
    } else if (command == "quit") {
        context.quitRequested = true;
    } else {
        print(context.console, "bilinmeyen komut: " + command + "   ('help' yaz)", ELine::kError);
    }
}

}  // namespace game::console
