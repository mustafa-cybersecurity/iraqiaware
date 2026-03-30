#include "ScreenshotManager.h"
#include "LoggingSystem.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <gdiplus.h>
#  pragma comment(lib, "gdiplus.lib")
#endif

namespace {

// ── Base64 encoding ───────────────────────────────────────────────────────────
static const char kB64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::vector<uint8_t>& data) {
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        uint32_t b = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < data.size()) b |= static_cast<uint32_t>(data[i + 1]) << 8;
        if (i + 2 < data.size()) b |= static_cast<uint32_t>(data[i + 2]);
        result += kB64Chars[(b >> 18) & 0x3F];
        result += kB64Chars[(b >> 12) & 0x3F];
        result += (i + 1 < data.size()) ? kB64Chars[(b >>  6) & 0x3F] : '=';
        result += (i + 2 < data.size()) ? kB64Chars[(b      ) & 0x3F] : '=';
    }
    return result;
}

// ── Current timestamp string for filenames ───────────────────────────────────
std::string timestampStr() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

ScreenshotManager::ScreenshotManager(int captureIntervalSeconds)
    : m_captureIntervalSec(captureIntervalSeconds)
    , m_outputDir(kOutputDir)
{
    ensureOutputDir();
}

ScreenshotManager::~ScreenshotManager() {
    stop();
}

void ScreenshotManager::start() {
    if (m_running.load()) return;
    m_running = true;
    m_thread  = std::thread(&ScreenshotManager::captureLoop, this);
    LOG_INFO("ScreenshotManager started (interval=" +
             std::to_string(m_captureIntervalSec) + "s)");
}

void ScreenshotManager::stop() {
    if (!m_running.load()) return;
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
    LOG_INFO("ScreenshotManager stopped. Total captures: " +
             std::to_string(m_captureCount.load()));
}

void ScreenshotManager::setCaptureCallback(CaptureCallback cb) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_callback = std::move(cb);
}

void ScreenshotManager::setCaptureInterval(int seconds) {
    m_captureIntervalSec = std::max(1, seconds);
}

std::string ScreenshotManager::getLastScreenshotPath() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_lastScreenshotPath;
}

// ── Background loop ───────────────────────────────────────────────────────────

void ScreenshotManager::captureLoop() {
    while (m_running.load()) {
        std::string filePath;
        std::string b64 = doCapture(filePath);

        if (!b64.empty()) {
            ++m_captureCount;
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                m_lastScreenshotPath = filePath;
                if (m_callback) m_callback(b64, filePath);
            }
        }

        // Sleep in small increments so stop() is responsive
        for (int i = 0; i < m_captureIntervalSec * 10 && m_running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

// ── Single capture ────────────────────────────────────────────────────────────

std::string ScreenshotManager::captureNow() {
    std::string filePath;
    return doCapture(filePath);
}

std::string ScreenshotManager::doCapture(std::string& outFilePath) {
#ifdef _WIN32
    // Initialise GDI+
    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr)
            != Gdiplus::Ok) {
        LOG_ERROR("GDI+ startup failed");
        return {};
    }

    // Get screen dimensions
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    // Create DC and compatible bitmap
    HDC     screenDC  = GetDC(nullptr);
    HDC     memDC     = CreateCompatibleDC(screenDC);
    HBITMAP hBitmap   = CreateCompatibleBitmap(screenDC, screenW, screenH);
    HGDIOBJ oldBmp    = SelectObject(memDC, hBitmap);
    BitBlt(memDC, 0, 0, screenW, screenH, screenDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);

    // Save bitmap to IStream as PNG via GDI+
    IStream* stream = nullptr;
    CreateStreamOnHGlobal(nullptr, TRUE, &stream);

    CLSID pngClsid;
    {
        // Find the PNG encoder CLSID
        UINT num = 0, sz = 0;
        Gdiplus::GetImageEncodersSize(&num, &sz);
        std::vector<uint8_t> buf(sz);
        auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
        Gdiplus::GetImageEncoders(num, sz, codecs);
        for (UINT i = 0; i < num; ++i) {
            if (wcscmp(codecs[i].MimeType, L"image/png") == 0) {
                pngClsid = codecs[i].Clsid;
                break;
            }
        }
    }

    Gdiplus::Bitmap bmp(hBitmap, nullptr);
    bmp.Save(stream, &pngClsid);

    // Read stream into buffer
    HGLOBAL hMem = nullptr;
    GetHGlobalFromStream(stream, &hMem);
    SIZE_T  memSize = GlobalSize(hMem);
    void*   pMem    = GlobalLock(hMem);
    std::vector<uint8_t> pngData(
        static_cast<uint8_t*>(pMem),
        static_cast<uint8_t*>(pMem) + memSize);
    GlobalUnlock(hMem);
    stream->Release();

    // Save PNG file to disk
    outFilePath = m_outputDir + "/screenshot_" + timestampStr() + ".png";
    {
        std::ofstream ofs(outFilePath, std::ios::binary);
        if (ofs.is_open()) ofs.write(
            reinterpret_cast<const char*>(pngData.data()),
            static_cast<std::streamsize>(pngData.size()));
    }

    // Cleanup
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
    Gdiplus::GdiplusShutdown(gdiplusToken);

    LOG_DEBUG("Screenshot captured → " + outFilePath);
    return base64Encode(pngData);

#else
    // Non-Windows stub: return empty so the app compiles on Linux/macOS
    LOG_WARNING("Screenshot capture is not supported on this platform.");
    outFilePath = "";
    return {};
#endif
}

// ── Helpers ───────────────────────────────────────────────────────────────────

std::string ScreenshotManager::toBase64(const std::vector<uint8_t>& data) {
    return base64Encode(data);
}

void ScreenshotManager::ensureOutputDir() {
    std::filesystem::create_directories(m_outputDir);
}
