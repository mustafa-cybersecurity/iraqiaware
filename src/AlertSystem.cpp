#include "AlertSystem.h"

#include <QApplication>
#include <QStyle>
#include <QAction>
#include <QDebug>

// ── Constructor / Destructor ──────────────────────────────────────────────────

AlertSystem::AlertSystem(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<AlertSystem::Alert>();
}

AlertSystem::~AlertSystem() = default;

// ── Public API ────────────────────────────────────────────────────────────────

void AlertSystem::initialise()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "AlertSystem: system tray not available on this desktop";
        return;
    }

    m_trayMenu = new QMenu();

    auto *showAction = new QAction(QStringLiteral("Show IraqiAware"), m_trayMenu);
    connect(showAction, &QAction::triggered,
            this, &AlertSystem::showWindowRequested);
    m_trayMenu->addAction(showAction);

    m_trayMenu->addSeparator();

    auto *quitAction = new QAction(QStringLiteral("Quit"), m_trayMenu);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    m_trayMenu->addAction(quitAction);

    m_trayIcon = new QSystemTrayIcon(
        QApplication::style()->standardIcon(QStyle::SP_ComputerIcon), this);
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setToolTip(QStringLiteral("IraqiAware – CyberSecurity Monitorizer"));

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this,        &AlertSystem::onTrayActivated);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked,
            this,        &AlertSystem::onTrayMessageClicked);

    m_trayIcon->show();
}

void AlertSystem::processThreats(const std::vector<SecurityAnalyzer::Threat> &threats)
{
    for (const auto &threat : threats) {
        Alert alert;
        alert.severity       = threat.severity;
        alert.category       = threat.category;
        alert.message        = threat.description;
        alert.recommendation = threat.recommendation;
        alert.timestamp      = threat.detectedAt;
        alert.acknowledged   = false;

        m_alerts.insert(m_alerts.begin(), std::move(alert));

        showTrayNotification(m_alerts.front());
        emit alertRaised(m_alerts.front());
    }

    emit unacknowledgedCountChanged(unacknowledgedCount());
    updateTrayTooltip();
}

void AlertSystem::acknowledgeAll()
{
    for (auto &a : m_alerts) a.acknowledged = true;
    emit unacknowledgedCountChanged(0);
    updateTrayTooltip();
}

const std::vector<AlertSystem::Alert> &AlertSystem::alerts() const
{
    return m_alerts;
}

int AlertSystem::unacknowledgedCount() const
{
    int count = 0;
    for (const auto &a : m_alerts)
        if (!a.acknowledged) ++count;
    return count;
}

bool AlertSystem::notificationsEnabled() const
{
    return m_notificationsEnabled;
}

void AlertSystem::setNotificationsEnabled(bool enabled)
{
    m_notificationsEnabled = enabled;
}

// ── Private slots ─────────────────────────────────────────────────────────────

void AlertSystem::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        emit showWindowRequested();
    }
}

void AlertSystem::onTrayMessageClicked()
{
    emit showWindowRequested();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void AlertSystem::showTrayNotification(const Alert &alert)
{
    if (!m_notificationsEnabled || !m_trayIcon) return;

    const QString title = QStringLiteral("[") +
                          SecurityAnalyzer::severityLabel(alert.severity) +
                          QStringLiteral("] ") + alert.category;

    m_trayIcon->showMessage(title, alert.message,
                            severityToIcon(alert.severity), 5000);
}

void AlertSystem::updateTrayTooltip()
{
    if (!m_trayIcon) return;
    const int n = unacknowledgedCount();
    const QString tip = n > 0
        ? QStringLiteral("IraqiAware – %1 unread alert(s)").arg(n)
        : QStringLiteral("IraqiAware – No unread alerts");
    m_trayIcon->setToolTip(tip);
}

QSystemTrayIcon::MessageIcon AlertSystem::severityToIcon(SecurityAnalyzer::Severity s)
{
    switch (s) {
    case SecurityAnalyzer::Severity::Critical:
    case SecurityAnalyzer::Severity::High:
        return QSystemTrayIcon::Critical;
    case SecurityAnalyzer::Severity::Medium:
        return QSystemTrayIcon::Warning;
    default:
        return QSystemTrayIcon::Information;
    }
}
