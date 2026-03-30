#pragma once

#include "ThreatModel.h"
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <memory>

// Forward declarations
class ScreenshotManager;
class AIServiceLayer;
class SecurityAnalyzer;
class ConfigManager;
class LanguageManager;
class AlertSystem;
class LoggingSystem;

// Qt forward declarations
QT_BEGIN_NAMESPACE
class QLabel;
class QTableWidget;
class QPushButton;
class QStackedWidget;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QTextEdit;
QT_END_NAMESPACE

/**
 * MainWindow
 * ----------
 * The primary application window.
 * Hosts a left-side navigation panel and a stacked widget that switches
 * between the Dashboard, Alerts, and Settings pages.
 *
 * Dark theme is loaded from resources/styles/dark_theme.qss at startup.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Navigation
    void showDashboard();
    void showAlerts();
    void showSettings();

    // Monitoring controls
    void toggleMonitoring();
    void onCaptureReady(const QString& base64, const QString& filePath);
    void onAnalysisComplete(bool success, const QString& response, const QString& error);
    void onThreatDetected(const std::vector<ThreatInfo>& threats);

    // Settings page
    void saveSettings();
    void testApiConnection();
    void onLanguageChanged(int index);
    void onProviderChanged(int index);

    // System tray
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void showFromTray();

    // Periodic UI refresh
    void refreshDashboard();

private:
    void setupUi();
    void setupDashboardPage();
    void setupAlertsPage();
    void setupSettingsPage();
    void setupSystemTray();
    void setupConnections();

    void loadStyleSheet();
    void applyLanguage();
    void updateDashboardStats();
    void addAlertRow(const ThreatInfo& threat);
    void showNotificationBanner(const ThreatInfo& threat);

    // ── Core components ──────────────────────────────────────────────────────
    std::unique_ptr<ConfigManager>    m_config;
    std::unique_ptr<LanguageManager>  m_lang;
    std::unique_ptr<ScreenshotManager>m_screenshot;
    std::unique_ptr<AIServiceLayer>   m_ai;
    std::unique_ptr<SecurityAnalyzer> m_analyzer;
    std::unique_ptr<AlertSystem>      m_alerts;

    // ── Qt UI elements ───────────────────────────────────────────────────────
    QStackedWidget* m_stack        = nullptr;

    // Dashboard page
    QLabel*   m_lblStatus          = nullptr;
    QLabel*   m_lblThreatCount     = nullptr;
    QLabel*   m_lblLastScan        = nullptr;
    QLabel*   m_lblScreenshots     = nullptr;
    QTextEdit*m_logOutput          = nullptr;

    // Alerts page
    QTableWidget* m_alertsTable    = nullptr;
    QPushButton*  m_btnExportJson  = nullptr;
    QPushButton*  m_btnExportCsv   = nullptr;
    QPushButton*  m_btnClearAlerts = nullptr;

    // Settings page
    QComboBox*  m_cmbLanguage      = nullptr;
    QComboBox*  m_cmbProvider      = nullptr;
    QLineEdit*  m_edtApiKey        = nullptr;
    QComboBox*  m_cmbModel         = nullptr;
    QSpinBox*   m_spnInterval      = nullptr;
    QCheckBox*  m_chkMonitoring    = nullptr;
    QCheckBox*  m_chkAlerts        = nullptr;
    QCheckBox*  m_chkSound         = nullptr;
    QPushButton*m_btnSaveSettings  = nullptr;
    QPushButton*m_btnTestApi       = nullptr;

    // Nav buttons
    QPushButton* m_btnNavDashboard = nullptr;
    QPushButton* m_btnNavAlerts    = nullptr;
    QPushButton* m_btnNavSettings  = nullptr;
    QPushButton* m_btnToggleMon    = nullptr;

    // System tray
    QSystemTrayIcon* m_trayIcon    = nullptr;

    // Refresh timer
    QTimer*          m_refreshTimer= nullptr;

    // Monitoring state
    bool             m_monitoring  = false;
};
