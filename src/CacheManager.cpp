#include "CacheManager.h"

#include <iomanip>
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────

CacheManager::CacheManager(size_t maxEntries, int ttlSeconds)
    : m_maxEntries(maxEntries)
    , m_ttlSeconds(ttlSeconds)
{}

// ── Mutating operations ───────────────────────────────────────────────────────

void CacheManager::store(const std::string& imageHash, const AnalysisResult& result) {
    std::lock_guard<std::mutex> lk(m_mutex);

    // Evict the oldest entry when the store is full
    if (m_cache.size() >= m_maxEntries) {
        auto oldest = m_cache.begin();
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->second.storedAt < oldest->second.storedAt)
                oldest = it;
        }
        m_cache.erase(oldest);
    }

    m_cache[imageHash] = {result, std::chrono::steady_clock::now()};
}

void CacheManager::evictExpired() {
    std::lock_guard<std::mutex> lk(m_mutex);
    for (auto it = m_cache.begin(); it != m_cache.end(); ) {
        if (isExpired(it->second))
            it = m_cache.erase(it);
        else
            ++it;
    }
}

void CacheManager::clear() {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_cache.clear();
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<AnalysisResult> CacheManager::get(const std::string& imageHash) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_cache.find(imageHash);
    if (it == m_cache.end() || isExpired(it->second))
        return std::nullopt;
    return it->second.result;
}

bool CacheManager::has(const std::string& imageHash) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_cache.find(imageHash);
    return it != m_cache.end() && !isExpired(it->second);
}

size_t CacheManager::size() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_cache.size();
}

// ── Static helpers ────────────────────────────────────────────────────────────

std::string CacheManager::computeHash(const std::string& data) {
    // FNV-1a 64-bit hash – fast and well-distributed for binary strings
    // Sample up to kHashSampleSize bytes evenly across the buffer for speed
    static constexpr size_t kHashSampleSize = 4096;
    uint64_t hash = 14695981039346656037ULL;
    const size_t step = (data.size() > kHashSampleSize)
                        ? (data.size() / kHashSampleSize)
                        : 1;
    for (size_t i = 0; i < data.size(); i += step) {
        hash ^= static_cast<unsigned char>(data[i]);
        hash *= 1099511628211ULL;
    }
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

bool CacheManager::isExpired(const Entry& e) const {
    const auto age = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - e.storedAt).count();
    return age > m_ttlSeconds;
}
