#include "ConfigManager.h"
#include "LoggingSystem.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

// ── Simple XOR + Base64 obfuscation (NOT cryptographic) ──────────────────────
namespace {
static const uint8_t kXorKey[] = { 0x4B, 0x3A, 0x7C, 0x29, 0xF1, 0xA3, 0x66, 0xD8 };
static const char kB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string b64Encode(const std::vector<uint8_t>& d) {
    std::string r;
    for (size_t i = 0; i < d.size(); i += 3) {
        uint32_t b = static_cast<uint32_t>(d[i]) << 16;
        if (i+1 < d.size()) b |= static_cast<uint32_t>(d[i+1]) << 8;
        if (i+2 < d.size()) b |= d[i+2];
        r += kB64[(b>>18)&63];
        r += kB64[(b>>12)&63];
        r += (i+1<d.size())?kB64[(b>>6)&63]:'=';
        r += (i+2<d.size())?kB64[b&63]:'=';
    }
    return r;
}

std::vector<uint8_t> b64Decode(const std::string& s) {
    auto val = [](char c) -> int {
        if (c>='A'&&c<='Z') return c-'A';
        if (c>='a'&&c<='z') return c-'a'+26;
        if (c>='0'&&c<='9') return c-'0'+52;
        if (c=='+') return 62;
        if (c=='/') return 63;
        return -1;
    };
    std::vector<uint8_t> r;
    for (size_t i = 0; i+3 < s.size(); i += 4) {
        int a=val(s[i]),b=val(s[i+1]),c=val(s[i+2]),d=val(s[i+3]);
        if (a<0||b<0) break;
        r.push_back(static_cast<uint8_t>((a<<2)|(b>>4)));
        if (c>=0) r.push_back(static_cast<uint8_t>((b<<4)|(c>>2)));
        if (d>=0) r.push_back(static_cast<uint8_t>((c<<6)|d));
    }
    return r;
}
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

ConfigManager::ConfigManager(const std::string& configFilePath)
    : m_configFilePath(configFilePath) {}

bool ConfigManager::load() {
    std::ifstream ifs(m_configFilePath);
    if (!ifs.is_open()) {
        LOG_WARNING("Config file not found at '" + m_configFilePath +
                    "' – using defaults");
        return false;
    }

    try {
        json j = json::parse(ifs);

        if (j.contains("app")) {
            auto& a = j["app"];
            m_cfg.appName    = a.value("name",     m_cfg.appName);
            m_cfg.appVersion = a.value("version",  m_cfg.appVersion);
            m_cfg.language   = a.value("language", m_cfg.language);
            m_cfg.theme      = a.value("theme",    m_cfg.theme);
        }
        if (j.contains("monitoring")) {
            auto& m = j["monitoring"];
            m_cfg.monitoringEnabled    = m.value("enabled",                m_cfg.monitoringEnabled);
            m_cfg.captureIntervalSec   = m.value("capture_interval_seconds",m_cfg.captureIntervalSec);
            m_cfg.maxScreenshotHistory = m.value("max_screenshots_history", m_cfg.maxScreenshotHistory);
        }
        if (j.contains("ai")) {
            auto& ai = j["ai"];
            m_cfg.aiProvider   = ai.value("provider",        m_cfg.aiProvider);
            m_cfg.aiApiKeyRaw  = ai.value("api_key",         m_cfg.aiApiKeyRaw);
            m_cfg.aiModel      = ai.value("model",           m_cfg.aiModel);
            m_cfg.aiTimeoutSec = ai.value("timeout_seconds", m_cfg.aiTimeoutSec);
        }
        if (j.contains("alerts")) {
            auto& al = j["alerts"];
            m_cfg.alertsEnabled       = al.value("enabled",            m_cfg.alertsEnabled);
            m_cfg.soundEnabled        = al.value("sound_enabled",       m_cfg.soundEnabled);
            m_cfg.notificationStyle   = al.value("notification_style",  m_cfg.notificationStyle);
        }
        if (j.contains("logging")) {
            auto& lg = j["logging"];
            m_cfg.logLevel          = lg.value("level",           m_cfg.logLevel);
            m_cfg.maxLogFileSizeMB  = lg.value("max_file_size_mb",m_cfg.maxLogFileSizeMB);
            m_cfg.logRetentionDays  = lg.value("retention_days",  m_cfg.logRetentionDays);
        }

        LOG_INFO("Configuration loaded from: " + m_configFilePath);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Config parse error: ") + e.what());
        return false;
    }
}

bool ConfigManager::save() const {
    json j;
    j["app"]["name"]     = m_cfg.appName;
    j["app"]["version"]  = m_cfg.appVersion;
    j["app"]["language"] = m_cfg.language;
    j["app"]["theme"]    = m_cfg.theme;

    j["monitoring"]["enabled"]                 = m_cfg.monitoringEnabled;
    j["monitoring"]["capture_interval_seconds"]= m_cfg.captureIntervalSec;
    j["monitoring"]["max_screenshots_history"] = m_cfg.maxScreenshotHistory;

    j["ai"]["provider"]        = m_cfg.aiProvider;
    j["ai"]["api_key"]         = m_cfg.aiApiKeyRaw;  // already obfuscated
    j["ai"]["model"]           = m_cfg.aiModel;
    j["ai"]["timeout_seconds"] = m_cfg.aiTimeoutSec;

    j["alerts"]["enabled"]            = m_cfg.alertsEnabled;
    j["alerts"]["sound_enabled"]      = m_cfg.soundEnabled;
    j["alerts"]["notification_style"] = m_cfg.notificationStyle;

    j["logging"]["level"]            = m_cfg.logLevel;
    j["logging"]["max_file_size_mb"] = m_cfg.maxLogFileSizeMB;
    j["logging"]["retention_days"]   = m_cfg.logRetentionDays;

    std::ofstream ofs(m_configFilePath);
    if (!ofs.is_open()) {
        LOG_ERROR("Cannot write config to: " + m_configFilePath);
        return false;
    }
    ofs << j.dump(2);
    return true;
}

// ── Getters / Setters ─────────────────────────────────────────────────────────

std::string ConfigManager::getAppName()    const { return m_cfg.appName; }
std::string ConfigManager::getAppVersion() const { return m_cfg.appVersion; }
std::string ConfigManager::getLanguage()   const { return m_cfg.language; }
std::string ConfigManager::getTheme()      const { return m_cfg.theme; }
void ConfigManager::setLanguage(const std::string& l) { m_cfg.language = l; }
void ConfigManager::setTheme(const std::string& t)    { m_cfg.theme    = t; }

bool ConfigManager::isMonitoringEnabled()     const { return m_cfg.monitoringEnabled; }
int  ConfigManager::getCaptureIntervalSec()   const { return m_cfg.captureIntervalSec; }
int  ConfigManager::getMaxScreenshotHistory() const { return m_cfg.maxScreenshotHistory; }
void ConfigManager::setMonitoringEnabled(bool e)   { m_cfg.monitoringEnabled  = e; }
void ConfigManager::setCaptureIntervalSec(int s)   { m_cfg.captureIntervalSec = s; }

std::string ConfigManager::getAIProvider()   const { return m_cfg.aiProvider; }
std::string ConfigManager::getAIModel()      const { return m_cfg.aiModel; }
int         ConfigManager::getAITimeoutSec() const { return m_cfg.aiTimeoutSec; }
void ConfigManager::setAIProvider(const std::string& p)  { m_cfg.aiProvider   = p; }
void ConfigManager::setAIModel(const std::string& m)     { m_cfg.aiModel      = m; }
void ConfigManager::setAITimeoutSec(int s)               { m_cfg.aiTimeoutSec = s; }

std::string ConfigManager::getAIApiKey() const {
    return deobfuscate(m_cfg.aiApiKeyRaw);
}

void ConfigManager::setAIApiKey(const std::string& key) {
    m_cfg.aiApiKeyRaw = obfuscate(key);
}

bool ConfigManager::isAlertsEnabled()             const { return m_cfg.alertsEnabled; }
bool ConfigManager::isSoundEnabled()              const { return m_cfg.soundEnabled; }
std::string ConfigManager::getNotificationStyle() const { return m_cfg.notificationStyle; }
void ConfigManager::setAlertsEnabled(bool e)            { m_cfg.alertsEnabled     = e; }
void ConfigManager::setSoundEnabled(bool e)             { m_cfg.soundEnabled      = e; }
void ConfigManager::setNotificationStyle(const std::string& s) { m_cfg.notificationStyle = s; }

std::string ConfigManager::getLogLevel()        const { return m_cfg.logLevel; }
int         ConfigManager::getMaxLogFileSizeMB()const { return m_cfg.maxLogFileSizeMB; }
int         ConfigManager::getLogRetentionDays()const { return m_cfg.logRetentionDays; }
void ConfigManager::setLogLevel(const std::string& l)  { m_cfg.logLevel = l; }

// ── Obfuscation ───────────────────────────────────────────────────────────────

std::string ConfigManager::obfuscate(const std::string& plain) {
    std::vector<uint8_t> buf(plain.begin(), plain.end());
    for (size_t i = 0; i < buf.size(); ++i)
        buf[i] ^= kXorKey[i % sizeof(kXorKey)];
    return b64Encode(buf);
}

std::string ConfigManager::deobfuscate(const std::string& encoded) {
    if (encoded.empty()) return {};
    auto buf = b64Decode(encoded);
    for (size_t i = 0; i < buf.size(); ++i)
        buf[i] ^= kXorKey[i % sizeof(kXorKey)];
    return std::string(buf.begin(), buf.end());
}
