#pragma once

#include "client/net/net_client.hpp"
#include "game/client/ui/console_window.hpp"
#include "game/client/input/input_map.hpp"
#include "game/client/input/input_sampler.hpp"
#include "game/client/interpolation/interpolation.hpp"
#include "game/client/prediction/prediction.hpp"

#include <string_view>

namespace game::console {

// Konsol komutlarının dokunabildiği şeyler.
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

// Bir satırı çalıştırır. Çıktı ve hatalar print ile konsola yazılır.
// Komutlar: help, clear, echo, binds, bind <aksiyon> <tuş>, sensitivity <değer>, cl_predict <0|1>, cl_interpolate <0|1>, cl_interp <saniye>, pawns, spectate <netId>, takeover, release, quit
void execute(Context_t& context, std::string_view line);

}  // namespace game::console
