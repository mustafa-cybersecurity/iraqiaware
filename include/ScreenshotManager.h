#pragma once

#include <QObject>
#include <QTimer>
#include <QPixmap>
#include <QDateTime>
#include <QString>
#include <functional>

/**
 * @brief Captures screenshots of the primary monitor at a configurable interval
 *        (default: every 5 seconds).
 *
 * Uses the Windows GDI / Qt screen-grab API so that the rest of the
 * application can stay platform-agnostic at the call-site.  The raw
 * pixel data is kept in memory and the latest JPEG bytes are forwarded
 * to registered listeners via the screenshotCaptured signal.
 */
class ScreenshotManager : public QObject
{
    Q_OBJECT

public:
    explicit ScreenshotManager(QObject *parent = nullptr);
    ~ScreenshotManager() override;

    /** Start periodic capture (interval defaults to 5 000 ms). */
    void start(int intervalMs = 5000);
    void stop();

    bool isRunning() const;
    int  intervalMs() const;

    /** Returns the most recent screenshot as raw JPEG bytes. */
    QByteArray latestScreenshotBytes() const;

    /** Returns the most recent screenshot as a QPixmap (for previewing). */
    QPixmap latestPixmap() const;

signals:
    /** Emitted after every successful capture.
     *  @param imageBytes  JPEG-encoded screenshot bytes ready to send to AI.
     *  @param timestamp When the screenshot was taken.
     */
    void screenshotCaptured(const QByteArray &imageBytes, const QDateTime &timestamp);
    void captureStarted();
    void captureStopped();

    /** Emitted when an error occurs during capture. */
    void captureError(const QString &errorMessage);

private slots:
    void onTimerTimeout();

private:
    QByteArray captureScreen();

    QTimer     m_timer;
    QPixmap    m_latestPixmap;
    QByteArray m_latestBytes;
    bool       m_running{false};
    int        m_intervalMs{5000};
};
