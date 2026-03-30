#pragma once

#include <string>
#include <map>

/**
 * LanguageManager
 * ---------------
 * Loads translation strings from JSON files (resources/translations/).
 * Supports English (en) and Arabic (ar) with runtime language switching.
 * Arabic text is rendered RTL by Qt automatically when the locale is set.
 */
class LanguageManager {
public:
    explicit LanguageManager(const std::string& translationsDir = "resources/translations");
    ~LanguageManager() = default;

    // Load translations for the given language code ("en" or "ar")
    bool loadLanguage(const std::string& langCode);

    // Current language code
    std::string currentLanguage() const { return m_currentLang; }

    // Look up a translation key (dot-separated path, e.g. "alerts.phishing")
    // Returns the key itself if not found
    std::string tr(const std::string& key) const;

    // Convenience alias
    std::string operator()(const std::string& key) const { return tr(key); }

    // Returns true when Arabic is active (useful for RTL layout adjustments)
    bool isRTL() const { return m_currentLang == "ar"; }

private:
    // Flatten nested JSON object into dot-separated key→value map
    static std::map<std::string, std::string>
    flattenJson(const std::string& jsonStr, const std::string& prefix = "");

    std::string m_translationsDir;
    std::string m_currentLang = "en";
    std::map<std::string, std::string> m_translations;
};
