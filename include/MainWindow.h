#pragma once

#include "SecurityAnalyzer.h"
#include "AlertSystem.h"
#include <QMainWindow>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTabWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <vector>

class ScreenshotManager;
class AIServiceLayer;
class SecurityAnalyzer;
class ConfigManager;
class LanguageManager;
class AlertSystem;

/**
 * @brief Main application window – Dark-themed Qt6 UI.
 *
 * Layout (tab-based):
 *  ┌─────────────────────────────────────────────────────┐
 *  │  IraqiAware – CyberSecurity Awareness Monitorizer   │
 *  ├──────────┬──────────────────────────────────────────┤
 *  │ Dashboard│ Alerts │ Settings │ About                │
 *  └──────────┴──────────────────────────────────────────┘
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(
        ScreenshotManager *screenshotMgr,
        AIServiceLayer    *aiService,
        SecurityAnalyzer  *analyzer,
        ConfigManager     *config,
        LanguageManager   *langMgr,
        AlertSystem       *alertSystem,
        QWidget           *parent = nullptr
    );
    ~MainWindow() override;

    void applyDarkTheme();
    void retranslateUi();

protected:
    void closeEvent(QCloseEvent *event) override;

public slots:
    void onScreenshotCaptured(const QByteArray &imageBytes, const QDateTime &timestamp);
    void onAnalysisComplete(const QString &analysisText);
    void onThreatsDetected(const std::vector<SecurityAnalyzer::Threat> &threats);
    void onNoThreatsDetected();
    void onAlertRaised(const AlertSystem::Alert &alert);
    void onCaptureError(const QString &errorMessage);
    void onAnalysisError(const QString &errorMessage);

private slots:
    void onStartStopClicked();
    void onSettingsSaveClicked();
    void onLanguageChanged(int index);
    void onProviderChanged(int index);
    void onClearAlertsClicked();
    void onShowWindowRequested();

private:
    // ── Build sub-pages ──────────────────────────────────────────────────────
    QWidget *buildDashboardPage();
    QWidget *buildAlertsPage();
    QWidget *buildSettingsPage();
    QWidget *buildAboutPage();

    void updateStatusBar(const QString &message);
    void addAlertRow(const AlertSystem::Alert &alert);
    void updateUnreadBadge(int count);
    QString providerName(int index) const;

    // ── Core components ──────────────────────────────────────────────────────
    ScreenshotManager *m_screenshotMgr;
    AIServiceLayer    *m_aiService;
    SecurityAnalyzer  *m_analyzer;
    ConfigManager     *m_config;
    LanguageManager   *m_langMgr;
    AlertSystem       *m_alertSystem;

    // ── UI widgets ───────────────────────────────────────────────────────────
    QTabWidget    *m_tabs{nullptr};

    // Dashboard
    QLabel        *m_screenshotPreview{nullptr};
    QTextEdit     *m_analysisOutput{nullptr};
    QPushButton   *m_startStopBtn{nullptr};
    QLabel        *m_statusLabel{nullptr};
    QProgressBar  *m_progressBar{nullptr};
    QLabel        *m_threatCountLabel{nullptr};

    // Alerts
    QListWidget   *m_alertList{nullptr};
    QPushButton   *m_clearAlertsBtn{nullptr};
    QLabel        *m_unreadBadge{nullptr};

    // Settings
    QComboBox     *m_providerCombo{nullptr};
    QLineEdit     *m_apiKeyEdit{nullptr};
    QLineEdit     *m_modelEdit{nullptr};
    QLineEdit     *m_customEndpointEdit{nullptr};
    QSpinBox      *m_intervalSpin{nullptr};
    QComboBox     *m_languageCombo{nullptr};
    QCheckBox     *m_notificationsCheck{nullptr};
    QCheckBox     *m_loggingCheck{nullptr};
    QPushButton   *m_saveSettingsBtn{nullptr};

    int  m_totalThreats{0};
    bool m_monitoring{false};
};
