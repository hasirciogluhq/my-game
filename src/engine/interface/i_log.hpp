#pragma once

#include <cstdint>
#include <format>
#include <optional>
#include <print>
#include <source_location>
#include <string>
#include <string_view>

namespace game {

// log.dll interface (spdlog, async). Each module uses only this header and the GAME_LOG_* macros.
// Auto-binding: GAME_EXPORT_MODULE() exports BindLog to every module; CModule::load binds the sink.
enum class ELogLevel : uint8_t { kTrace, kDebug, kInfo, kWarn, kError, kFatal, kOff };

#ifdef NDEBUG
inline constexpr ELogLevel kDefaultLogLevel = ELogLevel::kInfo;
#else
inline constexpr ELogLevel kDefaultLogLevel = ELogLevel::kDebug;
#endif

struct LogConfig_t {
    ELogLevel level = kDefaultLogLevel;
    std::string categoryLevels;  // "net:trace,render:warn"
    std::string file;            // empty = not written to a file
};

class ILog {
public:
    virtual ~ILog() = default;
    virtual void configure(const LogConfig_t& config) = 0;
    [[nodiscard]] virtual bool enabled(ELogLevel level, std::string_view category) const = 0;
    virtual void write(ELogLevel level, std::string_view category, std::string_view message, const std::source_location& where) = 0;
    virtual void flush() = 0;
};

inline constexpr char kLogInterface[] = "Log001";

[[nodiscard]] constexpr std::optional<ELogLevel> parseLogLevel(std::string_view text) {
    constexpr std::string_view kNames[]{"trace", "debug", "info", "warn", "error", "fatal", "off"};
    for (std::size_t i = 0; i < std::size(kNames); ++i) {
        if (text == kNames[i]) {
            return static_cast<ELogLevel>(i);
        }
    }
    return std::nullopt;
}

[[nodiscard]] constexpr std::string_view logLevelName(ELogLevel level) {
    constexpr std::string_view kNames[]{"trace", "debug", "info", "warn", "error", "fatal", "off"};
    return kNames[static_cast<std::size_t>(level)];
}

namespace logging {

// One sink per module (the static lib / header is embedded separately in each DLL; BindLog binds each one).
inline ILog* g_sink = nullptr;

inline void bind(ILog* sink) {
    g_sink = sink;
}

// Without a sink (log.dll not loaded / not bound yet) warn and above go to stderr.
[[nodiscard]] inline bool enabled(ELogLevel level, std::string_view category) {
    return g_sink != nullptr ? g_sink->enabled(level, category) : level >= ELogLevel::kWarn;
}

inline void write(ELogLevel level, std::string_view category, std::string_view message, const std::source_location& where) {
    if (g_sink != nullptr) {
        g_sink->write(level, category, message, where);
    } else {
        std::println(stderr, "[{}] [{}] {}", logLevelName(level), category, message);
    }
}

}  // namespace logging
}  // namespace game

// Usage: GAME_LOG_INFO("net", "port {} opened", port);
// If the level is disabled, std::format never runs.
#define GAME_LOG(level, category, ...)                                                                   \
    do {                                                                                                  \
        if (::game::logging::enabled(level, category)) {                                                  \
            ::game::logging::write(level, category, std::format(__VA_ARGS__), std::source_location::current()); \
        }                                                                                                 \
    } while (false)

#define GAME_LOG_TRACE(category, ...) GAME_LOG(::game::ELogLevel::kTrace, category, __VA_ARGS__)
#define GAME_LOG_DEBUG(category, ...) GAME_LOG(::game::ELogLevel::kDebug, category, __VA_ARGS__)
#define GAME_LOG_INFO(category, ...) GAME_LOG(::game::ELogLevel::kInfo, category, __VA_ARGS__)
#define GAME_LOG_WARN(category, ...) GAME_LOG(::game::ELogLevel::kWarn, category, __VA_ARGS__)
#define GAME_LOG_ERROR(category, ...) GAME_LOG(::game::ELogLevel::kError, category, __VA_ARGS__)
#define GAME_LOG_FATAL(category, ...) GAME_LOG(::game::ELogLevel::kFatal, category, __VA_ARGS__)
