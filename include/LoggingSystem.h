#pragma once

#include <string>
#include <memory>

/**
 * LoggingSystem
 * -------------
 * Thin wrapper around spdlog providing:
 *  - Rotating file sink (daily rotation)
 *  - Console (stdout) sink
 *  - Configurable log level
 *  - Structured log macros
 */
class LoggingSystem {
public:
    enum class Level { Debug, Info, Warning, Error, Critical };

    // Initialise logging (call once at application startup)
    static bool init(const std::string& logDir    = "logs",
                     const std::string& logFile   = "iraqiaware.log",
                     Level              minLevel   = Level::Info,
                     int                maxFileSizeMB = 10,
                     int                retentionDays = 7);

    // Shutdown and flush all sinks
    static void shutdown();

    // Log functions
    static void debug   (const std::string& msg);
    static void info    (const std::string& msg);
    static void warning (const std::string& msg);
    static void error   (const std::string& msg);
    static void critical(const std::string& msg);

    // Set log level at runtime
    static void setLevel(Level level);
    static void setLevel(const std::string& levelStr);

    // Convert string ("debug","info","warning","error","critical") → Level
    static Level levelFromString(const std::string& s);

private:
    static bool m_initialised;
};

// ── Convenience macros ────────────────────────────────────────────────────────
#define LOG_DEBUG(msg)    LoggingSystem::debug(msg)
#define LOG_INFO(msg)     LoggingSystem::info(msg)
#define LOG_WARNING(msg)  LoggingSystem::warning(msg)
#define LOG_ERROR(msg)    LoggingSystem::error(msg)
#define LOG_CRITICAL(msg) LoggingSystem::critical(msg)
