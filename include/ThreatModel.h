#pragma once

#include <string>
#include <vector>
#include <chrono>

// Forward declaration needed for Q_DECLARE_METATYPE
#include <QMetaType>

// Threat severity levels
enum class ThreatSeverity {
    Low,
    Medium,
    High,
    Critical
};

// Threat category types
enum class ThreatCategory {
    WeakPassword,
    PhishingDetected,
    SensitiveDataExposed,
    UnsafeBrowsing,
    MalwareIndicator,
    Unknown
};

// Represents a single detected security threat
struct ThreatInfo {
    std::string         id;
    ThreatCategory      category;
    ThreatSeverity      severity;
    std::string         title;
    std::string         description;
    std::string         recommendation;
    std::string         screenshotPath;
    std::chrono::system_clock::time_point detectedAt;
    bool                acknowledged = false;

    // Converts severity to human-readable string
    static std::string severityToString(ThreatSeverity s) {
        switch (s) {
            case ThreatSeverity::Low:      return "Low";
            case ThreatSeverity::Medium:   return "Medium";
            case ThreatSeverity::High:     return "High";
            case ThreatSeverity::Critical: return "Critical";
        }
        return "Unknown";
    }

    // Converts category to human-readable string
    static std::string categoryToString(ThreatCategory c) {
        switch (c) {
            case ThreatCategory::WeakPassword:        return "Weak Password";
            case ThreatCategory::PhishingDetected:    return "Phishing Detected";
            case ThreatCategory::SensitiveDataExposed:return "Sensitive Data Exposed";
            case ThreatCategory::UnsafeBrowsing:      return "Unsafe Browsing";
            case ThreatCategory::MalwareIndicator:    return "Malware Indicator";
            case ThreatCategory::Unknown:
            default:                                   return "Unknown";
        }
    }
};

// Aggregated analysis result from AI service
struct AnalysisResult {
    bool                     success = false;
    std::string              rawResponse;
    std::vector<ThreatInfo>  threats;
    std::string              errorMessage;
    std::chrono::system_clock::time_point analyzedAt;
};

// Monitoring statistics
struct MonitoringStats {
    uint64_t screenshotsCaptured  = 0;
    uint64_t analysesPerformed    = 0;
    uint64_t threatsDetected      = 0;
    uint64_t alertsSent           = 0;
    std::chrono::system_clock::time_point startedAt;
    std::chrono::system_clock::time_point lastScanAt;
};

// Qt metatype registration for queued signal/slot connections
Q_DECLARE_METATYPE(ThreatInfo)
Q_DECLARE_METATYPE(std::vector<ThreatInfo>)
