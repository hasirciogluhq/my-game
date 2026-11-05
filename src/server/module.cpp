#include "engine/interface/create_interface.hpp"
#include "server/server_app.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CServerApp, IServerApp, kServerAppInterface)

}  // namespace game

GAME_EXPORT_MODULE()
