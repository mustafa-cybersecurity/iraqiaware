#pragma once

#include <string>
#include <optional>

/**
 * ConfigManager
 * -------------
 * Reads and writes the application configuration stored in a JSON file.
 * API keys are obfuscated in storage (XOR + Base64) to avoid plain-text
 * secrets on disk.  This is NOT cryptographic security – use OS credential
 * stores for production deployments.
 */
class ConfigManager {
public:
    explicit ConfigManager(const std::string& configFilePath = "config/default_config.json");
    ~ConfigManager() = default;

    // Load config from disk; returns false on parse errors
    bool load();

    // Persist current configuration to disk
    bool save() const;

    // ── Application ──────────────────────────────────────────────────────────
    std::string getAppName()    const;
    std::string getAppVersion() const;
    std::string getLanguage()   const;
    std::string getTheme()      const;
    void setLanguage(const std::string& lang);
    void setTheme(const std::string& theme);

    // ── Monitoring ───────────────────────────────────────────────────────────
    bool isMonitoringEnabled()    const;
    int  getCaptureIntervalSec()  const;
    int  getMaxScreenshotHistory()const;
    void setMonitoringEnabled(bool enabled);
    void setCaptureIntervalSec(int seconds);

    // ── AI Provider ──────────────────────────────────────────────────────────
    std::string getAIProvider()   const;
    std::string getAIApiKey()     const;   // decrypted
    std::string getAIModel()      const;
    int         getAITimeoutSec() const;
    void setAIProvider(const std::string& provider);
    void setAIApiKey(const std::string& key);  // stored obfuscated
    void setAIModel(const std::string& model);
    void setAITimeoutSec(int seconds);

    // ── Alerts ───────────────────────────────────────────────────────────────
    bool isAlertsEnabled()     const;
    bool isSoundEnabled()      const;
    std::string getNotificationStyle() const;
    void setAlertsEnabled(bool enabled);
    void setSoundEnabled(bool enabled);
    void setNotificationStyle(const std::string& style);

    // ── Logging ──────────────────────────────────────────────────────────────
    std::string getLogLevel()       const;
    int         getMaxLogFileSizeMB()const;
    int         getLogRetentionDays()const;
    void setLogLevel(const std::string& level);

private:
    // Simple XOR-based obfuscation for API keys
    static std::string obfuscate(const std::string& plain);
    static std::string deobfuscate(const std::string& encoded);

    std::string m_configFilePath;

    // Cached values (populated by load())
    struct Config {
        // app
        std::string appName    = "CyberSecurity Awareness Monitorizer";
        std::string appVersion = "1.0.0";
        std::string language   = "en";
        std::string theme      = "dark";
        // monitoring
        bool monitoringEnabled   = true;
        int  captureIntervalSec  = 5;
        int  maxScreenshotHistory= 100;
        // ai
        std::string aiProvider    = "openai";
        std::string aiApiKeyRaw;   // obfuscated bytes on disk
        std::string aiModel       = "gpt-4o";
        int         aiTimeoutSec  = 30;
        // alerts
        bool alertsEnabled        = true;
        bool soundEnabled         = true;
        std::string notificationStyle = "toast";
        // logging
        std::string logLevel       = "info";
        int  maxLogFileSizeMB      = 10;
        int  logRetentionDays      = 7;
    } m_cfg;
};
