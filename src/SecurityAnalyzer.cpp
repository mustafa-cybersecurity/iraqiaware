#include "SecurityAnalyzer.h"
#include "LoggingSystem.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

using json = nlohmann::json;

// ── Public API ────────────────────────────────────────────────────────────────

SecurityAnalyzer::SecurityAnalyzer() = default;

AnalysisResult SecurityAnalyzer::parseAIResponse(const std::string& rawJson,
                                                   const std::string& screenshotPath) {
    AnalysisResult result;
    result.rawResponse = rawJson;
    result.analyzedAt  = std::chrono::system_clock::now();

    try {
        // The AI response may be wrapped in a Markdown code block
        std::string jsonStr = rawJson;
        auto start = jsonStr.find("```json");
        if (start != std::string::npos) {
            start += 7;
            auto end = jsonStr.find("```", start);
            if (end != std::string::npos) jsonStr = jsonStr.substr(start, end - start);
        } else {
            auto s2 = jsonStr.find("```");
            if (s2 != std::string::npos) {
                s2 += 3;
                auto e2 = jsonStr.find("```", s2);
                if (e2 != std::string::npos) jsonStr = jsonStr.substr(s2, e2 - s2);
            }
        }

        // Find the JSON object
        auto objStart = jsonStr.find('{');
        if (objStart != std::string::npos) jsonStr = jsonStr.substr(objStart);

        auto j = json::parse(jsonStr);
        if (!j.contains("threats") || !j["threats"].is_array()) {
            result.success = true; // valid response, no threats
            return result;
        }

        for (auto& item : j["threats"]) {
            ThreatInfo threat;
            threat.id             = generateId();
            threat.screenshotPath = screenshotPath;
            threat.detectedAt     = result.analyzedAt;

            threat.severity = parseSeverity(
                item.value("severity", "Medium"));
            threat.category = parseCategory(
                item.value("category", "Unknown"));
            threat.title          = item.value("title",          "Security Threat");
            threat.description    = item.value("description",    "");
            threat.recommendation = item.value("recommendation", "");

            result.threats.push_back(std::move(threat));
        }

        result.success = true;
        LOG_INFO("Analysis found " + std::to_string(result.threats.size()) + " threat(s)");

    } catch (const std::exception& e) {
        result.success      = false;
        result.errorMessage = std::string("Parse error: ") + e.what();
        LOG_ERROR("SecurityAnalyzer parse error: " + result.errorMessage);
    }

    // Store in history (FIFO)
    if (m_history.size() >= m_maxHistory) m_history.erase(m_history.begin());
    m_history.push_back(result);

    // Fire callback
    if (!result.threats.empty() && m_callback) {
        m_callback(result.threats);
    }

    return result;
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::vector<ThreatInfo> SecurityAnalyzer::getAllThreats() const {
    std::vector<ThreatInfo> all;
    for (auto it = m_history.rbegin(); it != m_history.rend(); ++it) {
        for (auto& t : it->threats) all.push_back(t);
    }
    return all;
}

std::vector<ThreatInfo> SecurityAnalyzer::getThreatsAboveSeverity(
        ThreatSeverity minSeverity) const {
    auto all = getAllThreats();
    all.erase(std::remove_if(all.begin(), all.end(), [&](const ThreatInfo& t) {
        return static_cast<int>(t.severity) < static_cast<int>(minSeverity);
    }), all.end());
    return all;
}

// ── Export ────────────────────────────────────────────────────────────────────

std::string SecurityAnalyzer::exportHistoryJson() const {
    json arr = json::array();
    for (auto& r : m_history) {
        json result;
        result["success"]   = r.success;
        result["error"]     = r.errorMessage;
        result["threats"]   = json::array();
        for (auto& t : r.threats) {
            result["threats"].push_back({
                {"id",             t.id},
                {"category",       ThreatInfo::categoryToString(t.category)},
                {"severity",       ThreatInfo::severityToString(t.severity)},
                {"title",          t.title},
                {"description",    t.description},
                {"recommendation", t.recommendation}
            });
        }
        arr.push_back(result);
    }
    return arr.dump(2);
}

std::string SecurityAnalyzer::exportHistoryCsv() const {
    std::ostringstream ss;
    ss << "ID,Category,Severity,Title,Description,Recommendation\n";
    for (auto& r : m_history) {
        for (auto& t : r.threats) {
            auto esc = [](std::string s) {
                size_t pos = 0;
                while ((pos = s.find('"', pos)) != std::string::npos) {
                    s.insert(pos, "\"");
                    pos += 2;
                }
                return "\"" + s + "\"";
            };
            ss << esc(t.id)                                       << ","
               << esc(ThreatInfo::categoryToString(t.category))  << ","
               << esc(ThreatInfo::severityToString(t.severity))  << ","
               << esc(t.title)                                    << ","
               << esc(t.description)                              << ","
               << esc(t.recommendation)                           << "\n";
        }
    }
    return ss.str();
}

// ── Private helpers ───────────────────────────────────────────────────────────

ThreatSeverity SecurityAnalyzer::parseSeverity(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "critical") return ThreatSeverity::Critical;
    if (lower == "high")     return ThreatSeverity::High;
    if (lower == "low")      return ThreatSeverity::Low;
    return ThreatSeverity::Medium;
}

ThreatCategory SecurityAnalyzer::parseCategory(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("weak") != std::string::npos ||
        lower.find("password") != std::string::npos) return ThreatCategory::WeakPassword;
    if (lower.find("phish") != std::string::npos)    return ThreatCategory::PhishingDetected;
    if (lower.find("sensitive") != std::string::npos ||
        lower.find("data") != std::string::npos)     return ThreatCategory::SensitiveDataExposed;
    if (lower.find("unsafe") != std::string::npos ||
        lower.find("browsing") != std::string::npos) return ThreatCategory::UnsafeBrowsing;
    if (lower.find("malware") != std::string::npos)  return ThreatCategory::MalwareIndicator;
    return ThreatCategory::Unknown;
}

std::string SecurityAnalyzer::generateId() {
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << dist(rng);
    return ss.str();
}
