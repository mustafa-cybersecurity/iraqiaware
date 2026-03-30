#pragma once

#include "ThreatModel.h"
#include <string>
#include <vector>
#include <functional>

/**
 * AlertSystem
 * -----------
 * Delivers user-facing notifications when threats are detected.
 *
 * On Windows:
 *  - System tray balloon tips (Shell_NotifyIcon)
 *  - Optional audible beep (MessageBeep / PlaySound)
 *
 * Maintains an in-memory alert log that the UI can query.
 */
class AlertSystem {
public:
    struct Alert {
        std::string  id;
        ThreatInfo   threat;
        bool         dismissed = false;
        std::chrono::system_clock::time_point createdAt;
    };

    explicit AlertSystem();
    ~AlertSystem();

    // Initialise the system-tray icon (call once after main window is shown)
    // hwnd: handle of the main application window (HWND cast to void*)
    bool initTrayIcon(void* hwnd);

    // Tear down tray icon on shutdown
    void cleanup();

    // Display an alert for the given threat
    void raiseAlert(const ThreatInfo& threat);

    // Raise alerts for a list of threats
    void raiseAlerts(const std::vector<ThreatInfo>& threats);

    // Mark an alert as dismissed by its id
    void dismissAlert(const std::string& alertId);

    // Dismiss all alerts
    void dismissAll();

    // Return all (including dismissed) alerts, newest first
    const std::vector<Alert>& getAllAlerts() const { return m_alerts; }

    // Return only undismissed alerts
    std::vector<Alert> getActiveAlerts() const;

    // Sound enable/disable
    void setSoundEnabled(bool enabled) { m_soundEnabled = enabled; }
    bool isSoundEnabled() const        { return m_soundEnabled; }

    // Notifications enable/disable
    void setNotificationsEnabled(bool enabled) { m_notifEnabled = enabled; }
    bool isNotificationsEnabled() const         { return m_notifEnabled; }

    // Export alert log to JSON string
    std::string exportAlertsJson() const;

    // Register a callback invoked whenever a new alert is raised
    using AlertCallback = std::function<void(const Alert&)>;
    void setAlertCallback(AlertCallback cb) { m_callback = std::move(cb); }

private:
    void playAlertSound(ThreatSeverity severity);
    void showTrayBalloon(const std::string& title, const std::string& msg,
                         ThreatSeverity severity);

    std::vector<Alert> m_alerts;
    AlertCallback      m_callback;
    bool               m_soundEnabled  = true;
    bool               m_notifEnabled  = true;
    bool               m_trayInitialised = false;
    void*              m_hwnd = nullptr;

    // Windows NOTIFYICONDATA handle (stored as opaque pointer)
    void*              m_nid = nullptr;
};
