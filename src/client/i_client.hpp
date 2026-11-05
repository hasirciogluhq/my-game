#pragma once

#include "engine/engine/core/launch_options.hpp"
#include "engine/interface/create_interface.hpp"

namespace game {

// client.dll arayüzü. connect: host'un factory'si (Renderer001, Particles001
// buradan alınır).
class IClientApp {
public:
  virtual ~IClientApp() = default;
  [[nodiscard]] virtual bool connect(CreateInterfaceFn factory) = 0;
  [[nodiscard]] virtual int run(const LaunchOptions_t &options) = 0;
};

inline constexpr char kClientAppInterface[] = "ClientApp001";

} // namespace game
