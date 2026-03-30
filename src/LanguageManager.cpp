#include "LanguageManager.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <fstream>

using json = nlohmann::json;

// ── Constructor / Destructor ──────────────────────────────────────────────────

LanguageManager::LanguageManager(QObject *parent)
    : QObject(parent)
{}

LanguageManager::~LanguageManager() = default;

// ── Public API ────────────────────────────────────────────────────────────────

bool LanguageManager::load(const QString &translationsDir)
{
    bool ok = true;
    ok &= loadFile(QStringLiteral("en"), translationsDir);
    ok &= loadFile(QStringLiteral("ar"), translationsDir);
    return ok;
}

bool LanguageManager::setLanguage(const QString &langCode)
{
    if (!m_tables.contains(langCode)) {
        qWarning() << "LanguageManager: unknown language" << langCode;
        return false;
    }
    m_currentLang = langCode;
    emit languageChanged(langCode);
    return true;
}

QString LanguageManager::currentLanguage() const
{
    return m_currentLang;
}

QString LanguageManager::tr(const QString &key) const
{
    auto it = m_tables.find(m_currentLang);
    if (it != m_tables.end()) {
        try {
            return QString::fromStdString(
                it->at(key.toStdString()).get<std::string>());
        } catch (...) {}
    }

    // Fallback to English
    auto enIt = m_tables.find(QStringLiteral("en"));
    if (enIt != m_tables.end()) {
        try {
            return QString::fromStdString(
                enIt->at(key.toStdString()).get<std::string>());
        } catch (...) {}
    }

    // Ultimate fallback: return the key itself
    return key;
}

QStringList LanguageManager::availableLanguages() const
{
    return m_tables.keys();
}

bool LanguageManager::isRtl() const
{
    return m_currentLang == QStringLiteral("ar");
}

// ── Private helpers ───────────────────────────────────────────────────────────

bool LanguageManager::loadFile(const QString &langCode, const QString &dir)
{
    const QString path = dir + QStringLiteral("/") + langCode + QStringLiteral(".json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "LanguageManager: cannot open" << path;
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    try {
        m_tables[langCode] = json::parse(data.constData());
    } catch (const json::parse_error &e) {
        qWarning() << "LanguageManager: JSON error in" << path << ":" << e.what();
        return false;
    }

    return true;
}
