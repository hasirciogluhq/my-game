#pragma once

#include "engine/engine/api.hpp"
#include "engine/interface/i_log.hpp"

namespace game {

// engine.dll dlopen ile yüklenmez (host linkler), bu yüzden BindLog export'u yoktur: host bunu çağırır.
ENGINE_API void bindEngineLog(ILog* sink);

}  // namespace game
