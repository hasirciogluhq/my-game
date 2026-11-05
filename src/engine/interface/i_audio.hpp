#pragma once

#include <string>

namespace game {

// audio.dll arayüzü (miniaudio). Ses aygıtı yoksa initialize false döner, oyun sessiz devam eder.
class IAudio {
public:
    virtual ~IAudio() = default;
    [[nodiscard]] virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    // Üretilmiş ses: dosya gerektirmez (test / geri bildirim sesleri).
    virtual void playTone(float frequencyHz, float seconds, float volume) = 0;
    // assets/ altındaki ses dosyasını çalar: "sounds/shot.wav".
    virtual void playFile(const std::string& path) = 0;
};

inline constexpr char kAudioInterface[] = "Audio001";

}  // namespace game
