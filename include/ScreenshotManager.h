#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>

/**
 * ScreenshotManager
 * -----------------
 * Captures full-screen screenshots using the Windows GDI+ API.
 * Operates on a dedicated background thread to avoid blocking the UI.
 * Captured images are converted to Base64 for transmission to the AI service.
 */
class ScreenshotManager {
public:
    // Callback type: invoked on the capture thread when a new screenshot
    // is ready.  Argument is the Base64-encoded PNG data.
    using CaptureCallback = std::function<void(const std::string& base64Image,
                                               const std::string& filePath)>;

    explicit ScreenshotManager(int captureIntervalSeconds = 5);
    ~ScreenshotManager();

    // Start / stop the background capture thread
    void start();
    void stop();

    bool isRunning() const { return m_running.load(); }

    // Register callback invoked after every successful capture
    void setCaptureCallback(CaptureCallback cb);

    // Configure capture frequency (seconds between captures)
    void setCaptureInterval(int seconds);
    int  getCaptureInterval() const { return m_captureIntervalSec; }

    // Returns path to the latest saved screenshot file
    std::string getLastScreenshotPath() const;

    // Trigger an immediate (synchronous) capture and return Base64 data
    std::string captureNow();

    // Total screenshots captured since start()
    uint64_t getCaptureCount() const { return m_captureCount.load(); }

    // Enable/disable delta detection.
    // When enabled, the capture callback is only invoked when the screen
    // content has changed since the previous capture (hash comparison).
    // Enabled by default.
    void setDeltaDetectionEnabled(bool enabled);
    bool isDeltaDetectionEnabled() const { return m_deltaDetection; }

private:
    // Background thread entry point
    void captureLoop();

    // Perform a single capture; returns Base64 string, saves to disk
    std::string doCapture(std::string& outFilePath);

    // Convert raw bitmap bytes to Base64
    static std::string toBase64(const std::vector<uint8_t>& data);

    // Ensure the screenshots output directory exists
    void ensureOutputDir();

    // Compute a fast hash over sampled bytes of a Base64 image string
    static std::string computeImageHash(const std::string& base64Data);

private:
    std::atomic<bool>  m_running{false};
    std::thread        m_thread;
    mutable std::mutex m_mutex;
    CaptureCallback    m_callback;
    int                m_captureIntervalSec;
    std::string        m_outputDir;
    std::string        m_lastScreenshotPath;
    std::atomic<uint64_t> m_captureCount{0};

    bool        m_deltaDetection   = true;
    std::string m_lastCaptureHash;

    static constexpr const char* kOutputDir = "screenshots";
};
