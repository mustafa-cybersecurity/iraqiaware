#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <nlohmann/json.hpp>

/**
 * @brief Provides English / Arabic UI string translations.
 *
 * Translation files are JSON objects mapping string keys to translated values
 * (resources/translations/en.json and ar.json).  Fallback to English when a
 * key is missing in the active language file.
 */
class LanguageManager : public QObject
{
    Q_OBJECT

public:
    explicit LanguageManager(QObject *parent = nullptr);
    ~LanguageManager() override;

    /** Load translation files from the given directory. */
    bool load(const QString &translationsDir);

    /** Switch active language ("en" or "ar"). */
    bool setLanguage(const QString &langCode);

    /** Active language code. */
    QString currentLanguage() const;

    /**
     * Look up @p key in the active translation table.
     * Returns @p key itself when no translation is found (fail-safe).
     */
    QString tr(const QString &key) const;

    /** List of language codes available in the translations directory. */
    QStringList availableLanguages() const;

    /** True if current language is RTL (right-to-left). */
    bool isRtl() const;

signals:
    void languageChanged(const QString &newLangCode);

private:
    bool loadFile(const QString &langCode, const QString &dir);

    QString                          m_currentLang{"en"};
    QHash<QString, nlohmann::json>   m_tables;   ///< langCode -> translation map
};
