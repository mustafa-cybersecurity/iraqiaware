#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

/**
 * @brief Loads, saves and exposes application configuration stored in JSON.
 *
 * On first run, default_config.json is copied to the user's config directory
 * (AppData/Local/IraqiAware on Windows).  All subsequent reads and writes use
 * the user copy so that the bundled defaults are never overwritten.
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager() override;

    /** Load configuration; call once at startup. */
    bool load();
    /** Persist current configuration to disk. */
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
