#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

// Forward declarations – avoid pulling in heavy headers
class AIServiceLayer;
class CacheManager;

/**
 * AsyncAnalyzer
 * -------------
 * Runs AIServiceLayer::analyzeScreenshot() on a dedicated background worker
 * thread so the Qt UI thread is never blocked during a network request.
 *
 * Tasks are queued serially (FIFO).  Before dispatching to the network the
 * manager checks CacheManager; a cache hit returns instantly without any
 * HTTP round-trip.
 *
 * The result callback is invoked from the worker thread.  Callers that need
 * to update Qt widgets must forward the result to the main thread via
 * QMetaObject::invokeMethod (see MainWindow::onCaptureReady).
 */
class AsyncAnalyzer {
public:
    // Mirrors AIServiceLayer::ResponseCallback
    using ResponseCallback = std::function<void(bool               success,
                                                const std::string& response,
                                                const std::string& error)>;

    explicit AsyncAnalyzer(AIServiceLayer& ai, CacheManager& cache);
    ~AsyncAnalyzer();

    // Non-copyable / non-movable (owns a thread)
    AsyncAnalyzer(const AsyncAnalyzer&)            = delete;
    AsyncAnalyzer& operator=(const AsyncAnalyzer&) = delete;

    /**
     * Enqueue an analysis task.
     * @param base64Image  Base64-encoded PNG/JPEG screenshot
     * @param callback     Invoked on the worker thread when done
     */
    void analyzeAsync(const std::string& base64Image,
                      ResponseCallback   callback);

    // True while the worker is processing a task
    bool isBusy() const { return m_busy.load(); }

    // Number of tasks waiting in the queue (not counting current)
    size_t pendingCount() const;

private:
    struct Task {
        std::string      base64Image;
        ResponseCallback callback;
    };

    void workerLoop();

    AIServiceLayer& m_ai;
    CacheManager&   m_cache;

    std::queue<Task>        m_queue;
    mutable std::mutex      m_mutex;
    std::condition_variable m_cv;
    std::thread             m_worker;
    std::atomic<bool>       m_stop{false};
    std::atomic<bool>       m_busy{false};
};
