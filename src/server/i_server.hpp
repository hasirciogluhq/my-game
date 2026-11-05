#pragma once

#include "engine/engine/core/launch_options.hpp"
#include "engine/interface/create_interface.hpp"

namespace game {

// server.dll interface.
class IServerApp {
public:
    virtual ~IServerApp() = default;
    [[nodiscard]] virtual bool connect(CreateInterfaceFn factory) = 0;
    [[nodiscard]] virtual int run(const LaunchOptions_t& options) = 0;
};

inline constexpr char kServerAppInterface[] = "ServerApp001";

}  // namespace game
