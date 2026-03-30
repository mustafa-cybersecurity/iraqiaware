#include "LoggingSystem.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <vector>

bool LoggingSystem::m_initialised = false;

static std::shared_ptr<spdlog::logger> g_logger;

// ─────────────────────────────────────────────────────────────────────────────

bool LoggingSystem::init(const std::string& logDir,
                          const std::string& logFile,
                          Level              minLevel,
                          int                maxFileSizeMB,
                          int                retentionDays) {
    if (m_initialised) return true;

    try {
        std::filesystem::create_directories(logDir);

        std::string logPath = logDir + "/" + logFile;
        std::size_t maxBytes = static_cast<std::size_t>(maxFileSizeMB) * 1024 * 1024;

        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logPath, maxBytes, static_cast<std::size_t>(retentionDays));

        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        g_logger = std::make_shared<spdlog::logger>(
            "iraqiaware",
            spdlog::sinks_init_list{fileSink, consoleSink});

        g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
        setLevel(minLevel);
        spdlog::register_logger(g_logger);
        spdlog::flush_every(std::chrono::seconds(3));

        m_initialised = true;
        g_logger->info("LoggingSystem initialised – log: {}", logPath);
        return true;

    } catch (const std::exception& e) {
        // Fallback: create a simple console logger so the app doesn't crash
        g_logger = spdlog::stdout_color_mt("iraqiaware_fallback");
        g_logger->error("Failed to initialise file logging: {}", e.what());
        m_initialised = true;
        return false;
    }
}

void LoggingSystem::shutdown() {
    if (g_logger) { g_logger->flush(); }
    spdlog::shutdown();
    m_initialised = false;
}

// ── Logging wrappers ──────────────────────────────────────────────────────────

void LoggingSystem::debug   (const std::string& msg) { if (g_logger) g_logger->debug(msg);    }
void LoggingSystem::info    (const std::string& msg) { if (g_logger) g_logger->info(msg);     }
void LoggingSystem::warning (const std::string& msg) { if (g_logger) g_logger->warn(msg);     }
void LoggingSystem::error   (const std::string& msg) { if (g_logger) g_logger->error(msg);    }
void LoggingSystem::critical(const std::string& msg) { if (g_logger) g_logger->critical(msg); }

// ── Level management ──────────────────────────────────────────────────────────

void LoggingSystem::setLevel(Level level) {
    if (!g_logger) return;
    switch (level) {
        case Level::Debug:    g_logger->set_level(spdlog::level::debug);   break;
        case Level::Info:     g_logger->set_level(spdlog::level::info);    break;
        case Level::Warning:  g_logger->set_level(spdlog::level::warn);    break;
        case Level::Error:    g_logger->set_level(spdlog::level::err);     break;
        case Level::Critical: g_logger->set_level(spdlog::level::critical);break;
    }
}

void LoggingSystem::setLevel(const std::string& levelStr) {
    setLevel(levelFromString(levelStr));
}

LoggingSystem::Level LoggingSystem::levelFromString(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "debug")    return Level::Debug;
    if (lower == "warning" || lower == "warn") return Level::Warning;
    if (lower == "error")    return Level::Error;
    if (lower == "critical") return Level::Critical;
    return Level::Info;
}
