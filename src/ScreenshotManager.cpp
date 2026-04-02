#include "ScreenshotManager.h"

#include <QApplication>
#include <QScreen>
#include <QBuffer>
#include <QPixmap>
#include <QGuiApplication>
#include <algorithm>

ScreenshotManager::ScreenshotManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout,
            this,     &ScreenshotManager::onTimerTimeout);
}

ScreenshotManager::~ScreenshotManager()
{
    stop();
}

void ScreenshotManager::start(int intervalMs)
{
    m_intervalMs = std::clamp(intervalMs, 250, 60000);
    m_running    = true;
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.start(m_intervalMs);
    emit captureStarted();
    onTimerTimeout(); // immediate first capture for real-time responsiveness
}

void ScreenshotManager::stop()
{
    m_running = false;
    m_timer.stop();
    emit captureStopped();
}

bool ScreenshotManager::isRunning() const
{
    return m_running;
}

int ScreenshotManager::intervalMs() const
{
    return m_intervalMs;
}

QByteArray ScreenshotManager::latestScreenshotBytes() const
{
    return m_latestBytes;
}

QPixmap ScreenshotManager::latestPixmap() const
{
    return m_latestPixmap;
}

// ── Private slots ─────────────────────────────────────────────────────────────

void ScreenshotManager::onTimerTimeout()
{
    QByteArray bytes = captureScreen();
    if (bytes.isEmpty()) {
        emit captureError(QStringLiteral("Failed to capture screen"));
        return;
    }
    m_latestBytes = bytes;
    emit screenshotCaptured(bytes, QDateTime::currentDateTime());
}

// ── Private helpers ───────────────────────────────────────────────────────────

QByteArray ScreenshotManager::captureScreen()
{
    // Use Qt's cross-platform screen grab.
    // On Windows this maps to BitBlt internally.
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return {};
    }

    QPixmap pixmap = screen->grabWindow(0);
    if (pixmap.isNull()) {
        return {};
    }

    m_latestPixmap = pixmap;

    QByteArray bytes;
    QBuffer    buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    // Encode as JPEG at quality 70 to reduce payload size sent to AI API.
    pixmap.save(&buffer, "JPEG", 70);
    buffer.close();

    return bytes;
}
