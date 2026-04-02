#pragma once

#include "ThreatModel.h"

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

/**
 * CacheManager
 * ------------
 * Thread-safe LRU cache for AI analysis results, keyed by a content hash of
 * the screenshot data.  Prevents re-analysing identical or near-identical
 * screenshots and allows the application to serve previous results instantly
 * when the AI service is unavailable.
 *
 * Eviction strategy:
 *  - Entries older than ttlSeconds are considered expired.
 *  - When the store is full the oldest entry is evicted first.
 */
class CacheManager {
public:
    explicit CacheManager(size_t maxEntries = 50, int ttlSeconds = 300);

    // Store a result for the given image hash
    void store(const std::string& imageHash, const AnalysisResult& result);

    // Retrieve a cached result; returns empty optional if absent or expired
    std::optional<AnalysisResult> get(const std::string& imageHash) const;

    // True if a valid (non-expired) entry exists for imageHash
    bool has(const std::string& imageHash) const;

    // Compute a stable 64-bit FNV-1a hex hash for the given data string
    static std::string computeHash(const std::string& data);

    // Remove all expired entries
    void evictExpired();

    // Remove all entries
    void clear();

    size_t size() const;

private:
    struct Entry {
        AnalysisResult                         result;
        std::chrono::steady_clock::time_point  storedAt;
    };

    bool isExpired(const Entry& e) const;

    mutable std::mutex                     m_mutex;
    std::unordered_map<std::string, Entry> m_cache;
    size_t                                 m_maxEntries;
    int                                    m_ttlSeconds;
};
