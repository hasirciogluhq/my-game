#pragma once

#include "engine/audio/api.hpp"
#include "engine/interface/i_audio.hpp"

#include <list>
#include <memory>

namespace game {

// miniaudio wrapper. Interface: IAudio (Audio001).
class AUDIO_API CAudioSystem final : public IAudio {
public:
    CAudioSystem();
    ~CAudioSystem() override;

    [[nodiscard]] bool initialize() override;
    void shutdown() override;
    void playTone(float frequencyHz, float seconds, float volume) override;
    void playFile(const std::string& path) override;

private:
    struct Impl_t;  // keep miniaudio types out of the header
    std::unique_ptr<Impl_t> m_impl;
};

}  // namespace game
