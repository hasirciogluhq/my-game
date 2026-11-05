#pragma once

#include "engine/engine/core/launch_options.hpp"
#include "engine/interface/create_interface.hpp"

namespace game {

// server.dll interface.
// Used by the dedicated_server host (open, then loop) and by test_game (open on the main thread, loop on a thread).
class IServerApp {
public:
    virtual ~IServerApp() = default;
    [[nodiscard]] virtual bool connect(CreateInterfaceFn factory) = 0;
    // Builds the world (map, rules, bots) and starts listening. False: nothing runs.
    [[nodiscard]] virtual bool open(const LaunchOptions_t& options) = 0;
    // The tick loop. Blocks; the process ends with it.
    virtual void loop() = 0;
};

inline constexpr char kServerAppInterface[] = "ServerApp001";

}  // namespace game
