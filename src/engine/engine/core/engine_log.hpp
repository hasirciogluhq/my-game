#pragma once

#include "engine/engine/api.hpp"
#include "engine/interface/i_log.hpp"

namespace game {

// engine.dll is not loaded with dlopen (the host links it), so it has no BindLog export: the host calls this instead.
ENGINE_API void bindEngineLog(ILog* sink);

}  // namespace game
