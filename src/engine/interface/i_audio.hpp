#pragma once

#include <string>

namespace game {

// audio.dll interface (miniaudio). If there is no audio device, initialize returns false and the game continues silently.
class IAudio {
public:
    virtual ~IAudio() = default;
    [[nodiscard]] virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    // Generated sound: needs no file (test / feedback sounds).
    virtual void playTone(float frequencyHz, float seconds, float volume) = 0;
    // Plays a sound file under assets/: "sounds/shot.wav".
    virtual void playFile(const std::string& path) = 0;
};

inline constexpr char kAudioInterface[] = "Audio001";

}  // namespace game
