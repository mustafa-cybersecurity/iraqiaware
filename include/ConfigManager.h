#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

/**
 * @brief Loads, saves and exposes application configuration stored in JSON.
 *
 * Configuration is loaded from bundled defaults and kept in memory only.
 * Runtime changes are applied for the current session and are not persisted
 * to disk to preserve user privacy.
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager() override;

    /** Load configuration; call once at startup. */
    bool load();
    /** Privacy mode: emits configSaved without writing to disk. */
    bool save();

    // ── Typed accessors ──────────────────────────────────────────────────────
    QString  apiKey(const QString &provider) const;
    void     setApiKey(const QString &provider, const QString &key);

    QString  activeProvider() const;
    void     setActiveProvider(const QString &provider);

    QString  activeModel() const;
    void     setActiveModel(const QString &model);

    QString  customEndpoint() const;
    void     setCustomEndpoint(const QString &url);

    int      screenshotIntervalMs() const;
    void     setScreenshotIntervalMs(int ms);

    QString  language() const;
    void     setLanguage(const QString &langCode);   ///< "en" or "ar"

    bool     notificationsEnabled() const;
    void     setNotificationsEnabled(bool enabled);

    bool     loggingEnabled() const;
    void     setLoggingEnabled(bool enabled);

    QString  externalApiWebhookUrl() const;
    void     setExternalApiWebhookUrl(const QString &url);

    QString  externalApiKey() const;
    void     setExternalApiKey(const QString &key);

    bool     externalApiEnabled() const;
    void     setExternalApiEnabled(bool enabled);

    /** Generic get/set for arbitrary keys (dot-separated path). */
    QVariant value(const QString &key, const QVariant &defaultValue = {}) const;
    void     setValue(const QString &key, const QVariant &value);

    /** Full path to the active config file. */
    QString configFilePath() const;

signals:
    void configChanged(const QString &key);
    void configLoaded();
    void configSaved();

private:
    void     ensureUserConfigExists();
    QString  userConfigDir() const;

    nlohmann::json m_config;
    QString        m_configPath;
    bool           m_loaded{false};
};
