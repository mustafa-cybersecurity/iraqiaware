#pragma once

#include "ThreatModel.h"
#include <string>
#include <vector>
#include <functional>

/**
 * SecurityAnalyzer
 * ----------------
 * Parses raw AI-model responses and extracts structured ThreatInfo objects.
 * Also maintains an in-memory history of recent analysis results.
 */
class SecurityAnalyzer {
public:
    // Callback fired whenever new threats are detected
    using ThreatCallback = std::function<void(const std::vector<ThreatInfo>& newThreats)>;

    explicit SecurityAnalyzer();
    ~SecurityAnalyzer() = default;

    // Parse raw JSON returned by AIServiceLayer → AnalysisResult
    AnalysisResult parseAIResponse(const std::string& rawJson,
                                   const std::string& screenshotPath = "");

    // Register callback for detected threats
    void setThreatCallback(ThreatCallback cb) { m_callback = std::move(cb); }

    // Access the full analysis history
    const std::vector<AnalysisResult>& getHistory() const { return m_history; }

    // Clear history
    void clearHistory() { m_history.clear(); }

    // Maximum number of results kept in history (FIFO eviction)
    void setMaxHistory(size_t n) { m_maxHistory = n; }

    // Return all threats across the full history, newest first
    std::vector<ThreatInfo> getAllThreats() const;

    // Return threats filtered by minimum severity
    std::vector<ThreatInfo> getThreatsAboveSeverity(ThreatSeverity minSeverity) const;

    // Export history to JSON string
    std::string exportHistoryJson() const;

    // Export history to CSV string
    std::string exportHistoryCsv() const;

private:
    // Convert AI severity string → enum
    static ThreatSeverity parseSeverity(const std::string& s);

    // Convert AI category string → enum
    static ThreatCategory parseCategory(const std::string& s);

    // Generate a short unique ID for a threat
    static std::string generateId();

    std::vector<AnalysisResult> m_history;
    ThreatCallback              m_callback;
    size_t                      m_maxHistory = 100;
};
