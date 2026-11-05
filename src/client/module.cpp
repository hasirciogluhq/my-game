#include "client/client_app.hpp"
#include "engine/interface/create_interface.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CClientApp, IClientApp, kClientAppInterface)

} // namespace game

GAME_EXPORT_MODULE()
