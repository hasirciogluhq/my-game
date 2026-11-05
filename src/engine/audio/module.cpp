#include "engine/audio/audio_system.hpp"
#include "engine/interface/create_interface.hpp"

namespace game {

GAME_EXPOSE_INTERFACE(CAudioSystem, IAudio, kAudioInterface)

}  // namespace game

GAME_EXPORT_MODULE()
