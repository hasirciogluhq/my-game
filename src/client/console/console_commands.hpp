#pragma once

#include "client/net/net_client.hpp"
#include "game/client/ui/console_window.hpp"
#include "game/client/input/input_map.hpp"
#include "game/client/input/input_sampler.hpp"
#include "game/client/interpolation/interpolation.hpp"
#include "game/client/prediction/prediction.hpp"

#include <string_view>

namespace game::console {

// What the console commands can touch.
struct Context_t {
    ui::Console_t& console;
    Bindings_t& bindings;
    CInputSampler& input;
    prediction::Prediction_t& prediction;
    interpolation::Interpolation_t& interpolation;
    bool& quitRequested;
    CNetClient& network;
    const World_t& world;
};

// Runs one line. Output and errors are written to the console with print.
// Commands: help, clear, echo, binds, bind <action> <key>, sensitivity <value>, cl_predict <0|1>, cl_interpolate <0|1>, cl_interp <seconds>, pawns, spectate <netId>, takeover, release, quit
void execute(Context_t& context, std::string_view line);

}  // namespace game::console
