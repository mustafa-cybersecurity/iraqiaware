#pragma once

#include "SecurityAnalyzer.h"
#include <QObject>
#include <QString>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QNetworkAccessManager>
#include <vector>

/**
 * @brief Delivers real-time security threat notifications to the user.
 *
 * Notifications are displayed as:
 *  - System-tray balloon messages (Windows toast via QSystemTrayIcon).
 *  - An in-app alert log kept in memory (accessible via alerts()).
 *
 * The system-tray icon is created here so the application can run minimised
 * and still surface critical alerts.
 */
class AlertSystem : public QObject
{
    Q_OBJECT

public:
    struct Alert {
        SecurityAnalyzer::Severity severity;
        QString category;
        QString message;
        QString recommendation;
        QDateTime timestamp;
        bool     acknowledged{false};
    };

    explicit AlertSystem(QObject *parent = nullptr);
    ~AlertSystem() override;

    /** Initialise the system-tray icon (call after QApplication is created). */
    void initialise();

    /** Process a list of threats and emit alerts for each one. */
    void processThreats(const std::vector<SecurityAnalyzer::Threat> &threats);

    /** Acknowledge (dismiss) all unread alerts. */
    void acknowledgeAll();

    /** Returns all alerts since application start, newest first. */
    const std::vector<Alert> &alerts() const;

    /** Count of unacknowledged alerts. */
    int unacknowledgedCount() const;

    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);
    void configureExternalApi(bool enabled, const QString &webhookUrl, const QString &apiKey);

signals:
    /** Emitted for each individual alert shown. */
    void alertRaised(const AlertSystem::Alert &alert);

    /** Emitted when the unacknowledged count changes. */
    void unacknowledgedCountChanged(int count);

    /** Emitted when the user clicks "Show" in the tray notification. */
    void showWindowRequested();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayMessageClicked();

private:
    void showTrayNotification(const Alert &alert);
    void forwardAlertToExternalApi(const Alert &alert);
    void updateTrayTooltip();
    static QSystemTrayIcon::MessageIcon severityToIcon(SecurityAnalyzer::Severity s);

    QSystemTrayIcon *m_trayIcon{nullptr};
    QMenu           *m_trayMenu{nullptr};
    QNetworkAccessManager m_networkManager;
    std::vector<Alert> m_alerts;
    bool             m_notificationsEnabled{true};
    bool             m_externalApiEnabled{false};
    QString          m_externalApiWebhookUrl;
    QString          m_externalApiKey;
};

Q_DECLARE_METATYPE(AlertSystem::Alert)
