#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "engine/audio/audio_system.hpp"

#include "engine/engine/core/asset_path.hpp"
#include "engine/interface/i_log.hpp"

namespace game {

struct CAudioSystem::Impl_t {
    struct Tone_t {
        ma_waveform wave;
        ma_sound sound;
    };

    ma_engine engine;
    bool ready = false;
    std::list<std::unique_ptr<Tone_t>> tones;  // finished tones are cleaned up on every playTone
};

CAudioSystem::CAudioSystem() : m_impl(std::make_unique<Impl_t>()) {}

CAudioSystem::~CAudioSystem() {
    shutdown();
}

bool CAudioSystem::initialize() {
    if (m_impl->ready) {
        return true;
    }
    if (ma_engine_init(nullptr, &m_impl->engine) != MA_SUCCESS) {
        GAME_LOG_WARN("audio", "audio device could not be opened, continuing silently");
        return false;
    }
    m_impl->ready = true;
    return true;
}

void CAudioSystem::shutdown() {
    if (!m_impl->ready) {
        return;
    }
    for (auto& tone : m_impl->tones) {
        ma_sound_uninit(&tone->sound);
    }
    m_impl->tones.clear();
    ma_engine_uninit(&m_impl->engine);
    m_impl->ready = false;
}

void CAudioSystem::playTone(float frequencyHz, float seconds, float volume) {
    if (!m_impl->ready) {
        return;
    }
    std::erase_if(m_impl->tones, [](auto& tone) {
        if (ma_sound_is_playing(&tone->sound) != 0) {
            return false;
        }
        ma_sound_uninit(&tone->sound);
        return true;
    });

    auto tone = std::make_unique<Impl_t::Tone_t>();
    const ma_waveform_config config = ma_waveform_config_init(
        ma_format_f32, ma_engine_get_channels(&m_impl->engine), ma_engine_get_sample_rate(&m_impl->engine),
        ma_waveform_type_square, 1.0, frequencyHz);
    if (ma_waveform_init(&config, &tone->wave) != MA_SUCCESS ||
        ma_sound_init_from_data_source(&m_impl->engine, &tone->wave, 0, nullptr, &tone->sound) != MA_SUCCESS) {
        return;
    }
    ma_sound_set_volume(&tone->sound, volume);
    const auto now = ma_engine_get_time_in_milliseconds(&m_impl->engine);
    ma_sound_set_stop_time_in_milliseconds(&tone->sound, now + static_cast<ma_uint64>(seconds * 1000.0f));
    ma_sound_start(&tone->sound);
    m_impl->tones.push_back(std::move(tone));
}

void CAudioSystem::playFile(const std::string& path) {
    if (!m_impl->ready) {
        return;
    }
    const auto file = findAssetPath(path);
    if (!file) {
        GAME_LOG_WARN("audio", "audio file not found: {}", path);
        return;
    }
    ma_engine_play_sound(&m_impl->engine, file->string().c_str(), nullptr);
}

}  // namespace game
