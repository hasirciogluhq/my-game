#include "engine/engine/core/engine_log.hpp"

namespace game {

void bindEngineLog(ILog* sink) {
    logging::bind(sink);
}

}  // namespace game
