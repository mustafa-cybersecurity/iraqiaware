#include "ConfigManager.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

// ── Constructor / Destructor ──────────────────────────────────────────────────

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{}

ConfigManager::~ConfigManager() = default;

// ── Public API ────────────────────────────────────────────────────────────────

bool ConfigManager::load()
{
    ensureUserConfigExists();

    std::ifstream ifs(m_configPath.toStdString());
    if (!ifs.is_open()) {
        qWarning() << "ConfigManager: cannot open" << m_configPath;
        return false;
    }

    try {
        ifs >> m_config;
    } catch (const json::parse_error &e) {
        qWarning() << "ConfigManager: JSON parse error:" << e.what();
        return false;
    }

    m_loaded = true;
    emit configLoaded();
    return true;
}

bool ConfigManager::save()
{
    // Privacy-first mode: keep configuration in memory only.
    emit configSaved();
    return true;
}

// ── Typed accessors ───────────────────────────────────────────────────────────

QString ConfigManager::apiKey(const QString &provider) const
{
    try {
        return QString::fromStdString(
            m_config.at("api_keys").at(provider.toStdString()).get<std::string>());
    } catch (...) {
        return {};
    }
}

void ConfigManager::setApiKey(const QString &provider, const QString &key)
{
    m_config["api_keys"][provider.toStdString()] = key.toStdString();
    emit configChanged(QStringLiteral("api_keys.") + provider);
}

QString ConfigManager::activeProvider() const
{
    try {
        return QString::fromStdString(
            m_config.at("active_provider").get<std::string>());
    } catch (...) {
        return QStringLiteral("openai");
    }
}

void ConfigManager::setActiveProvider(const QString &provider)
{
    m_config["active_provider"] = provider.toStdString();
    emit configChanged(QStringLiteral("active_provider"));
}

QString ConfigManager::activeModel() const
{
    try {
        return QString::fromStdString(
            m_config.at("active_model").get<std::string>());
    } catch (...) {
        return QStringLiteral("gpt-4o");
    }
}

void ConfigManager::setActiveModel(const QString &model)
{
    m_config["active_model"] = model.toStdString();
    emit configChanged(QStringLiteral("active_model"));
}

QString ConfigManager::customEndpoint() const
{
    try {
        return QString::fromStdString(
            m_config.at("custom_endpoint").get<std::string>());
    } catch (...) {
        return {};
    }
}

void ConfigManager::setCustomEndpoint(const QString &url)
{
    m_config["custom_endpoint"] = url.toStdString();
    emit configChanged(QStringLiteral("custom_endpoint"));
}

int ConfigManager::screenshotIntervalMs() const
{
    try {
        return m_config.at("screenshot_interval_ms").get<int>();
    } catch (...) {
        return 5000;
    }
}

void ConfigManager::setScreenshotIntervalMs(int ms)
{
    m_config["screenshot_interval_ms"] = ms;
    emit configChanged(QStringLiteral("screenshot_interval_ms"));
}

QString ConfigManager::language() const
{
    try {
        return QString::fromStdString(m_config.at("language").get<std::string>());
    } catch (...) {
        return QStringLiteral("en");
    }
}

void ConfigManager::setLanguage(const QString &langCode)
{
    m_config["language"] = langCode.toStdString();
    emit configChanged(QStringLiteral("language"));
}

bool ConfigManager::notificationsEnabled() const
{
    try {
        return m_config.at("notifications_enabled").get<bool>();
    } catch (...) {
        return true;
    }
}

void ConfigManager::setNotificationsEnabled(bool enabled)
{
    m_config["notifications_enabled"] = enabled;
    emit configChanged(QStringLiteral("notifications_enabled"));
}

bool ConfigManager::loggingEnabled() const
{
    try {
        return m_config.at("logging_enabled").get<bool>();
    } catch (...) {
        return true;
    }
}

void ConfigManager::setLoggingEnabled(bool enabled)
{
    m_config["logging_enabled"] = enabled;
    emit configChanged(QStringLiteral("logging_enabled"));
}

QString ConfigManager::externalApiWebhookUrl() const
{
    try {
        return QString::fromStdString(m_config.at("external_api").at("webhook_url").get<std::string>());
    } catch (...) {
        return {};
    }
}

void ConfigManager::setExternalApiWebhookUrl(const QString &url)
{
    m_config["external_api"]["webhook_url"] = url.toStdString();
    emit configChanged(QStringLiteral("external_api.webhook_url"));
}

QString ConfigManager::externalApiKey() const
{
    try {
        return QString::fromStdString(m_config.at("external_api").at("api_key").get<std::string>());
    } catch (...) {
        return {};
    }
}

void ConfigManager::setExternalApiKey(const QString &key)
{
    m_config["external_api"]["api_key"] = key.toStdString();
    emit configChanged(QStringLiteral("external_api.api_key"));
}

bool ConfigManager::externalApiEnabled() const
{
    try {
        return m_config.at("external_api").at("enabled").get<bool>();
    } catch (...) {
        return false;
    }
}

void ConfigManager::setExternalApiEnabled(bool enabled)
{
    m_config["external_api"]["enabled"] = enabled;
    emit configChanged(QStringLiteral("external_api.enabled"));
}

QVariant ConfigManager::value(const QString &key, const QVariant &defaultValue) const
{
    try {
        const json &v = m_config.at(key.toStdString());
        if (v.is_string())  return QString::fromStdString(v.get<std::string>());
        if (v.is_number_integer()) return v.get<int>();
        if (v.is_number_float())   return v.get<double>();
        if (v.is_boolean())        return v.get<bool>();
    } catch (...) {}
    return defaultValue;
}

void ConfigManager::setValue(const QString &key, const QVariant &val)
{
    const std::string k = key.toStdString();
    switch (static_cast<int>(val.typeId())) {
    case QMetaType::Bool:   m_config[k] = val.toBool();   break;
    case QMetaType::Int:    m_config[k] = val.toInt();    break;
    case QMetaType::Double: m_config[k] = val.toDouble(); break;
    default:                m_config[k] = val.toString().toStdString(); break;
    }
    emit configChanged(key);
}

QString ConfigManager::configFilePath() const
{
    return m_configPath;
}

// ── Private helpers ───────────────────────────────────────────────────────────

void ConfigManager::ensureUserConfigExists()
{
    // Privacy-first mode: always read bundled defaults, never write user config.
    QString defaultPath = QCoreApplication::applicationDirPath()
                          + QStringLiteral("/config/default_config.json");
    if (!QFile::exists(defaultPath)) {
        // Fallback for development environments
        defaultPath = QDir::currentPath() + QStringLiteral("/config/default_config.json");
    }
    m_configPath = defaultPath;
}

QString ConfigManager::userConfigDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}
