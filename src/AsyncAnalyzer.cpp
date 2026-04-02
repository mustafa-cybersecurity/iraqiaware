#include "AsyncAnalyzer.h"
#include "AIServiceLayer.h"
#include "CacheManager.h"
#include "LoggingSystem.h"

// ─────────────────────────────────────────────────────────────────────────────

AsyncAnalyzer::AsyncAnalyzer(AIServiceLayer& ai, CacheManager& cache)
    : m_ai(ai)
    , m_cache(cache)
{
    m_worker = std::thread(&AsyncAnalyzer::workerLoop, this);
    LOG_INFO("AsyncAnalyzer started");
}

AsyncAnalyzer::~AsyncAnalyzer() {
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_stop = true;
    }
    m_cv.notify_one();
    if (m_worker.joinable())
        m_worker.join();
    LOG_INFO("AsyncAnalyzer stopped");
}

// ── Public API ────────────────────────────────────────────────────────────────

void AsyncAnalyzer::analyzeAsync(const std::string& base64Image,
                                  ResponseCallback   callback) {
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.push({base64Image, std::move(callback)});
    }
    m_cv.notify_one();
}

size_t AsyncAnalyzer::pendingCount() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_queue.size();
}

// ── Worker loop ───────────────────────────────────────────────────────────────

void AsyncAnalyzer::workerLoop() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cv.wait(lk, [this] {
                return !m_queue.empty() || m_stop.load();
            });
            if (m_stop.load() && m_queue.empty())
                break;
            task = std::move(m_queue.front());
            m_queue.pop();
        }

        m_busy = true;

        // ── Cache lookup ──────────────────────────────────────────────────────
        const std::string hash = CacheManager::computeHash(task.base64Image);
        auto cached = m_cache.get(hash);
        if (cached.has_value()) {
            LOG_DEBUG("AsyncAnalyzer: cache hit (hash=" + hash + ")");
            if (task.callback)
                task.callback(cached->success,
                              cached->rawResponse,
                              cached->errorMessage);
            m_busy = false;
            continue;
        }

        // ── Network request ───────────────────────────────────────────────────
        bool        outOk   = false;
        std::string outResp;
        std::string outErr;

        m_ai.analyzeScreenshot(
            task.base64Image,
            [&outOk, &outResp, &outErr](bool ok,
                                         const std::string& resp,
                                         const std::string& err) {
                outOk   = ok;
                outResp = resp;
                outErr  = err;
            });

        // Store successful results in cache
        if (outOk) {
            AnalysisResult r;
            r.success     = true;
            r.rawResponse = outResp;
            m_cache.store(hash, r);
        }

        if (task.callback)
            task.callback(outOk, outResp, outErr);

        m_busy = false;
    }
}
