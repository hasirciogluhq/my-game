// log.dll: ILog (Log001) on top of spdlog (async). Single file: the class, registration and module export are all here.
#include "engine/interface/i_log.hpp"
#include "engine/interface/create_interface.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

namespace game {
namespace {

constexpr std::size_t kQueueSize = 8192;
constexpr std::size_t kMaxFileBytes = 5 * 1024 * 1024;
constexpr std::size_t kMaxFiles = 3;

spdlog::level::level_enum toSpdlog(ELogLevel level) {
    switch (level) {
        case ELogLevel::kTrace: return spdlog::level::trace;
        case ELogLevel::kDebug: return spdlog::level::debug;
        case ELogLevel::kInfo: return spdlog::level::info;
        case ELogLevel::kWarn: return spdlog::level::warn;
        case ELogLevel::kError: return spdlog::level::err;
        case ELogLevel::kFatal: return spdlog::level::critical;
        case ELogLevel::kOff: break;
    }
    return spdlog::level::off;
}

// "net:trace,render:warn" -> [{"net", kTrace}, {"render", kWarn}]. Invalid entries are skipped.
std::vector<std::pair<std::string, ELogLevel>> parseCategories(std::string_view text) {
    std::vector<std::pair<std::string, ELogLevel>> result;
    while (!text.empty()) {
        const std::size_t comma = text.find(',');
        const std::string_view item = text.substr(0, comma);
        text = comma == std::string_view::npos ? std::string_view{} : text.substr(comma + 1);
        const std::size_t colon = item.find(':');
        if (colon == std::string_view::npos) {
            continue;
        }
        if (const auto level = parseLogLevel(item.substr(colon + 1))) {
            result.emplace_back(std::string(item.substr(0, colon)), *level);
        }
    }
    return result;
}

class CLogSystem final : public ILog {
public:
    // No spdlog::shutdown() here: other modules may still log while the process ends (see game::exitProcess).
    ~CLogSystem() override {
        flush();
    }

    void configure(const LogConfig_t& config) override {
        static bool threadPoolReady = false;
        if (!threadPoolReady) {
            spdlog::init_thread_pool(kQueueSize, 1);
            threadPoolReady = true;
        }

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        if (!config.file.empty()) {
            const std::filesystem::path file{config.file};
            if (file.has_parent_path()) {
                std::error_code ignored;
                std::filesystem::create_directories(file.parent_path(), ignored);
            }
            sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config.file, kMaxFileBytes, kMaxFiles));
        }

        auto logger = std::make_shared<spdlog::async_logger>("game", sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                             spdlog::async_overflow_policy::overrun_oldest);
        logger->set_pattern("[%H:%M:%S.%e] [%l] [%t] %v");
        logger->set_level(spdlog::level::trace);  // filtering happens inside enabled()
        logger->flush_on(spdlog::level::err);
        // Flush once per second: even if the process is killed by a signal, everything except the last second stays in the log file.
        spdlog::drop("game");
        spdlog::register_logger(logger);
        spdlog::flush_every(std::chrono::seconds(1));

        m_level = config.level;
        m_categories = parseCategories(config.categoryLevels);
        m_logger = std::move(logger);
    }

    [[nodiscard]] bool enabled(ELogLevel level, std::string_view category) const override {
        ELogLevel threshold = m_level;
        for (const auto& [name, categoryLevel] : m_categories) {
            if (name == category) {
                threshold = categoryLevel;
                break;
            }
        }
        return level >= threshold && threshold != ELogLevel::kOff;
    }

    void write(ELogLevel level, std::string_view category, std::string_view message, const std::source_location& where) override {
        const std::string file = std::filesystem::path(where.file_name()).filename().string();
        const std::string line = std::format("[{}] {}:{} {}", category, file, where.line(), message);
        if (m_logger != nullptr) {
            m_logger->log(toSpdlog(level), line);
        } else {
            std::println(stderr, "[{}] {}", logLevelName(level), line);
        }
        if (level == ELogLevel::kFatal) {
            flush();
            std::abort();
        }
    }

    void flush() override {
        if (m_logger != nullptr) {
            m_logger->flush();
        }
    }

private:
    ELogLevel m_level = kDefaultLogLevel;
    std::vector<std::pair<std::string, ELogLevel>> m_categories;
    std::shared_ptr<spdlog::logger> m_logger;
};

GAME_EXPOSE_INTERFACE(CLogSystem, ILog, kLogInterface)

}  // namespace
}  // namespace game

GAME_EXPORT_MODULE()
