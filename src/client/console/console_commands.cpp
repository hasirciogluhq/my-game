#include "client/console/console_commands.hpp"

#include <exception>
#include <format>
#include <sstream>
#include <string>

namespace game::console {
using namespace game::ui;
namespace {

void printHelp(Console_t& console) {
    print(console, "help                   this list");
    print(console, "clear                  clear the console");
    print(console, "echo <text>            print the text");
    print(console, "binds                  list the key bindings");
    print(console, "bind <action> <key>    bind an action to a key (saved)");
    print(console, "sensitivity <value>    mouse sensitivity (radians/pixel)");
    print(console, "cl_predict <0|1>       turn client-side prediction on/off");
    print(console, "cl_interpolate <0|1>   turn interpolation on/off");
    print(console, "cl_interp <seconds>    interpolation delay (0.0-0.5)");
    print(console, "pawns                  list the characters (netId, who drives it)");
    print(console, "spectate <netId>       watch a character (releases your own)");
    print(console, "takeover               take over the bot you are watching");
    print(console, "release                stop watching / release the bot, return to your own character");
    print(console, "quit                   quit the game");
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
        print(context.console, "usage: bind <action> <key>   (see the actions with 'binds')", ELine::kError);
        return;
    }
    const auto action = input::actionFromName(actionText);
    if (!action) {
        print(context.console, "unknown action: " + actionText, ELine::kError);
        return;
    }
    input::bind(context.bindings, *action, key);
    input::save(context.bindings, input::configPath());
    print(context.console, std::format("{} -> {}", actionText, key));
}

void runSensitivity(Context_t& context, std::istringstream& words) {
    float value = 0.0f;
    if (!(words >> value) || value <= 0.0f) {
        print(context.console, "usage: sensitivity <positive number>   (default 0.0025)", ELine::kError);
        return;
    }
    context.input.setMouseSensitivity(value);
    print(context.console, std::format("sensitivity = {}", value));
}

void runPredict(Context_t& context, std::istringstream& words) {
    int value = 0;
    if (!(words >> value) || (value != 0 && value != 1)) {
        print(context.console, std::format("usage: cl_predict <0|1>   (currently {})", context.prediction.enabled ? 1 : 0), ELine::kError);
        return;
    }
    context.prediction.enabled = value == 1;
    prediction::clear(context.prediction);  // the old prediction history is no longer valid
    print(context.console, std::format("cl_predict = {}", value));
}

void runInterpolate(Context_t& context, std::istringstream& words) {
    int value = 0;
    if (!(words >> value) || (value != 0 && value != 1)) {
        print(context.console, std::format("usage: cl_interpolate <0|1>   (currently {})", context.interpolation.enabled ? 1 : 0), ELine::kError);
        return;
    }
    context.interpolation.enabled = value == 1;
    print(context.console, std::format("cl_interpolate = {}", value));
}

void runInterp(Context_t& context, std::istringstream& words) {
    float value = 0.0f;
    if (!(words >> value) || value < 0.0f || value > 0.5f) {
        print(context.console, std::format("usage: cl_interp <0.0-0.5>   (currently {})", context.interpolation.delaySeconds), ELine::kError);
        return;
    }
    context.interpolation.delaySeconds = value;
    print(context.console, std::format("cl_interp = {}", value));
}

void runPawns(const Context_t& context) {
    const uint32_t me = context.network.playerId().value_or(0);
    for (const auto [entity, player, replicated] : context.world.registry.view<const Player_t, const Replicated_t>().each()) {
        const std::string driver = player.playerId == 0 ? "bot/empty" : (player.playerId == me ? "you" : std::format("player {}", player.playerId));
        print(context.console, std::format("  netId {:<4} {}", replicated.netId, driver));
    }
}

void runSpectate(Context_t& context, std::istringstream& words) {
    uint32_t netId = 0;
    if (!(words >> netId) || netId == 0) {
        print(context.console, "usage: spectate <netId>   (see the netIds with 'pawns')", ELine::kError);
        return;
    }
    context.network.sendControl({.action = EControlAction::kSpectate, .targetNetId = netId});
    print(context.console, std::format("spectate requested: {}", netId));
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
        runControl(context, EControlAction::kTakeOver, "takeover requested (only for the bot being watched)");
    } else if (command == "release") {
        runControl(context, EControlAction::kRelease, "release requested");
    } else if (command == "quit") {
        context.quitRequested = true;
    } else {
        print(context.console, "unknown command: " + command + "   (type 'help')", ELine::kError);
    }
}

}  // namespace game::console
