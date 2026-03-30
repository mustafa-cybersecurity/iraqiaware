#include "LanguageManager.h"
#include "LoggingSystem.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────

LanguageManager::LanguageManager(const std::string& translationsDir)
    : m_translationsDir(translationsDir) {}

bool LanguageManager::loadLanguage(const std::string& langCode) {
    std::string path = m_translationsDir + "/" + langCode + ".json";
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        LOG_ERROR("Translation file not found: " + path);
        return false;
    }

    try {
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        m_translations = flattenJson(content);
        m_currentLang  = langCode;
        LOG_INFO("Loaded language: " + langCode + " (" +
                 std::to_string(m_translations.size()) + " strings)");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("LanguageManager parse error: ") + e.what());
        return false;
    }
}

std::string LanguageManager::tr(const std::string& key) const {
    auto it = m_translations.find(key);
    return (it != m_translations.end()) ? it->second : key;
}

// ── Flatten nested JSON → dot-separated keys ─────────────────────────────────

std::map<std::string, std::string>
LanguageManager::flattenJson(const std::string& jsonStr,
                               const std::string& prefix) {
    std::map<std::string, std::string> result;
    auto j = json::parse(jsonStr);

    std::function<void(const json&, const std::string&)> flatten =
        [&](const json& node, const std::string& pfx) {
            if (node.is_object()) {
                for (auto& [k, v] : node.items()) {
                    std::string fullKey = pfx.empty() ? k : pfx + "." + k;
                    flatten(v, fullKey);
                }
            } else if (node.is_string()) {
                result[pfx] = node.get<std::string>();
            } else {
                result[pfx] = node.dump();
            }
        };

    flatten(j, prefix);
    return result;
}
