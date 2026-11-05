#pragma once

#include "engine/engine/api.hpp"
#include "engine/interface/i_log.hpp"

namespace game {

// engine.dll is not loaded with dlopen (the host links it), so it has no BindLog export: the host calls this instead.
ENGINE_API void bindEngineLog(ILog* sink);

// Process end for the hosts. Flushes the log, then leaves WITHOUT running static destructors: modules log from their
// destructors, and by then the log thread pool is gone (async spdlog) -> abort. Use it as the last call of main().
[[noreturn]] ENGINE_API void exitProcess(ILog* log, int code);

}  // namespace game
