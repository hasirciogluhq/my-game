#include "engine/engine/core/engine_log.hpp"

#include <cstdio>
#include <cstdlib>

namespace game {

void bindEngineLog(ILog* sink) {
    logging::bind(sink);
}

void exitProcess(ILog* log, int code) {
    if (log != nullptr) {
        log->flush();
    }
    std::fflush(nullptr);
    std::_Exit(code);
}

}  // namespace game
