#pragma once

#include "engine/engine/core/launch_options.hpp"
#include "engine/interface/create_interface.hpp"

namespace game {

// client.dll interface. connect: the host's factory (Renderer001 and Particles001
// are taken from there).
class IClientApp {
public:
  virtual ~IClientApp() = default;
  [[nodiscard]] virtual bool connect(CreateInterfaceFn factory) = 0;
  [[nodiscard]] virtual int run(const LaunchOptions_t &options) = 0;
};

inline constexpr char kClientAppInterface[] = "ClientApp001";

} // namespace game
