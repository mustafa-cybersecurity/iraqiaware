#include "MainWindow.h"
#include "ScreenshotManager.h"
#include "AIServiceLayer.h"
#include "SecurityAnalyzer.h"
#include "ConfigManager.h"
#include "LanguageManager.h"
#include "AlertSystem.h"

#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QFrame>
#include <QSplitter>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileInfo>
#include <QPixmap>
#include <QDateTime>
#include <QFont>
#include <QSizePolicy>
#include <QDebug>

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(
    ScreenshotManager *screenshotMgr,
    AIServiceLayer    *aiService,
    SecurityAnalyzer  *analyzer,
    ConfigManager     *config,
    LanguageManager   *langMgr,
    AlertSystem       *alertSystem,
    QWidget           *parent)
    : QMainWindow(parent)
    , m_screenshotMgr(screenshotMgr)
    , m_aiService(aiService)
    , m_analyzer(analyzer)
    , m_config(config)
    , m_langMgr(langMgr)
    , m_alertSystem(alertSystem)
{
    setWindowTitle(QStringLiteral("IraqiAware – CyberSecurity Awareness Monitorizer"));
    setMinimumSize(960, 640);
    resize(1100, 700);

    // ── Central widget + tab bar ─────────────────────────────────────────────
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildDashboardPage(), QStringLiteral("🖥  Dashboard"));
    m_tabs->addTab(buildAlertsPage(),    QStringLiteral("🔔  Alerts"));
    m_tabs->addTab(buildSettingsPage(),  QStringLiteral("⚙   Settings"));
    m_tabs->addTab(buildAboutPage(),     QStringLiteral("ℹ   About"));
    setCentralWidget(m_tabs);

    // ── Status bar ───────────────────────────────────────────────────────────
    statusBar()->showMessage(QStringLiteral("Ready – configure your AI API key in Settings to begin."));

    // ── Signal connections ───────────────────────────────────────────────────
    connect(m_screenshotMgr, &ScreenshotManager::screenshotCaptured,
            this,            &MainWindow::onScreenshotCaptured);
    connect(m_screenshotMgr, &ScreenshotManager::captureError,
            this,            &MainWindow::onCaptureError);
    connect(m_aiService, &AIServiceLayer::analysisComplete,
            this,        &MainWindow::onAnalysisComplete);
    connect(m_aiService, &AIServiceLayer::analysisError,
            this,        &MainWindow::onAnalysisError);
    connect(m_analyzer, &SecurityAnalyzer::threatsDetected,
            this,       &MainWindow::onThreatsDetected);
    connect(m_analyzer, &SecurityAnalyzer::noThreatsDetected,
            this,       &MainWindow::onNoThreatsDetected);
    connect(m_alertSystem, &AlertSystem::alertRaised,
            this,          &MainWindow::onAlertRaised);
    connect(m_alertSystem, &AlertSystem::showWindowRequested,
            this,          &MainWindow::onShowWindowRequested);

    // ── Forward screenshot to AI ─────────────────────────────────────────────
    connect(m_screenshotMgr, &ScreenshotManager::screenshotCaptured,
            this, [this](const QByteArray &bytes, const QDateTime &) {
                m_aiService->analyzeScreenshot(bytes);
            });

    // ── Forward AI result to analyzer ────────────────────────────────────────
    connect(m_aiService, &AIServiceLayer::analysisComplete,
            m_analyzer,  &SecurityAnalyzer::analyze);

    // ── Forward threats to alert system ─────────────────────────────────────
    connect(m_analyzer, &SecurityAnalyzer::threatsDetected,
            m_alertSystem, &AlertSystem::processThreats);

    applyDarkTheme();
    retranslateUi();
}

MainWindow::~MainWindow() = default;

// ── Dark theme ────────────────────────────────────────────────────────────────

void MainWindow::applyDarkTheme()
{
    // Read QSS file if it exists, otherwise use inline style
    QFile qssFile(QApplication::applicationDirPath()
                  + QStringLiteral("/resources/styles/dark_theme.qss"));

    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
        return;
    }

    // Inline fallback dark stylesheet
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget {
            background-color: #1e1e2e;
            color: #cdd6f4;
            font-family: "Segoe UI", sans-serif;
            font-size: 13px;
        }
        QTabWidget::pane { border: 1px solid #313244; }
        QTabBar::tab {
            background: #181825;
            color: #cdd6f4;
            padding: 8px 18px;
            border: 1px solid #313244;
            border-bottom: none;
        }
        QTabBar::tab:selected { background: #1e1e2e; color: #89b4fa; }
        QTabBar::tab:hover { background: #313244; }
        QPushButton {
            background-color: #313244;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 4px;
            padding: 6px 14px;
        }
        QPushButton:hover { background-color: #45475a; }
        QPushButton:pressed { background-color: #585b70; }
        QPushButton#startBtn { background-color: #a6e3a1; color: #1e1e2e; font-weight: bold; }
        QPushButton#startBtn:hover { background-color: #94e2d5; }
        QPushButton#stopBtn { background-color: #f38ba8; color: #1e1e2e; font-weight: bold; }
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox {
            background-color: #181825;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 4px;
            padding: 4px;
        }
        QLineEdit:focus, QTextEdit:focus { border-color: #89b4fa; }
        QComboBox {
            background-color: #181825;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 4px;
            padding: 4px;
        }
        QComboBox QAbstractItemView { background-color: #181825; color: #cdd6f4; }
        QGroupBox {
            border: 1px solid #313244;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 4px;
            color: #89b4fa;
            font-weight: bold;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; }
        QListWidget {
            background-color: #181825;
            border: 1px solid #313244;
            alternate-background-color: #1e1e2e;
        }
        QListWidget::item { padding: 6px; }
        QListWidget::item:hover { background-color: #313244; }
        QListWidget::item:selected { background-color: #45475a; }
        QLabel#screenshotLabel {
            background-color: #11111b;
            border: 1px solid #313244;
            border-radius: 4px;
        }
        QProgressBar {
            background-color: #313244;
            border-radius: 4px;
            text-align: center;
        }
        QProgressBar::chunk { background-color: #89b4fa; border-radius: 4px; }
        QScrollBar:vertical {
            background: #181825;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background: #45475a;
            border-radius: 5px;
        }
        QStatusBar { background-color: #181825; color: #a6adc8; }
        QCheckBox { spacing: 6px; }
        QCheckBox::indicator {
            width: 14px; height: 14px;
            background: #181825;
            border: 1px solid #45475a;
            border-radius: 2px;
        }
        QCheckBox::indicator:checked { background-color: #89b4fa; }
    )"));
}

// ── retranslateUi ─────────────────────────────────────────────────────────────

void MainWindow::retranslateUi()
{
    if (!m_langMgr) return;
    setWindowTitle(m_langMgr->tr("app_title"));

    const bool rtl = m_langMgr->isRtl();
    QApplication::setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);

    if (m_startStopBtn) {
        m_startStopBtn->setText(
            m_monitoring ? m_langMgr->tr("btn_stop_monitoring")
                         : m_langMgr->tr("btn_start_monitoring"));
    }
    if (m_clearAlertsBtn)
        m_clearAlertsBtn->setText(m_langMgr->tr("btn_clear_alerts"));
    if (m_saveSettingsBtn)
        m_saveSettingsBtn->setText(m_langMgr->tr("btn_save_settings"));
    if (m_statusLabel)
        m_statusLabel->setText(m_langMgr->tr("status_idle"));
}

// ── Build dashboard page ──────────────────────────────────────────────────────

QWidget *MainWindow::buildDashboardPage()
{
    auto *page   = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    // ── Left panel: screenshot preview ──────────────────────────────────────
    auto *leftPanel  = new QGroupBox(QStringLiteral("Screen Preview"));
    auto *leftLayout = new QVBoxLayout(leftPanel);

    m_screenshotPreview = new QLabel;
    m_screenshotPreview->setObjectName(QStringLiteral("screenshotLabel"));
    m_screenshotPreview->setAlignment(Qt::AlignCenter);
    m_screenshotPreview->setMinimumSize(480, 270);
    m_screenshotPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_screenshotPreview->setText(QStringLiteral("No screenshot yet…"));
    leftLayout->addWidget(m_screenshotPreview);

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat(QStringLiteral("AI Analysis: %p%"));
    m_progressBar->setFixedHeight(18);
    leftLayout->addWidget(m_progressBar);

    connect(m_aiService, &AIServiceLayer::requestProgress,
            m_progressBar, &QProgressBar::setValue);

    // Control buttons
    auto *btnLayout = new QHBoxLayout;
    m_startStopBtn = new QPushButton(QStringLiteral("▶  Start Monitoring"));
    m_startStopBtn->setObjectName(QStringLiteral("startBtn"));
    m_startStopBtn->setMinimumHeight(36);
    connect(m_startStopBtn, &QPushButton::clicked,
            this,           &MainWindow::onStartStopClicked);
    btnLayout->addWidget(m_startStopBtn);

    m_statusLabel = new QLabel(QStringLiteral("Status: Idle"));
    m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    btnLayout->addWidget(m_statusLabel);
    leftLayout->addLayout(btnLayout);

    layout->addWidget(leftPanel, 3);

    // ── Right panel: AI analysis output ─────────────────────────────────────
    auto *rightPanel  = new QGroupBox(QStringLiteral("AI Security Analysis"));
    auto *rightLayout = new QVBoxLayout(rightPanel);

    m_threatCountLabel = new QLabel(QStringLiteral("Threats detected: 0"));
    m_threatCountLabel->setAlignment(Qt::AlignRight);
    rightLayout->addWidget(m_threatCountLabel);

    m_analysisOutput = new QTextEdit;
    m_analysisOutput->setReadOnly(true);
    m_analysisOutput->setPlaceholderText(
        QStringLiteral("AI analysis results will appear here after monitoring starts…"));
    m_analysisOutput->setMinimumWidth(340);
    rightLayout->addWidget(m_analysisOutput);

    layout->addWidget(rightPanel, 2);

    return page;
}

// ── Build alerts page ─────────────────────────────────────────────────────────

QWidget *MainWindow::buildAlertsPage()
{
    auto *page   = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    // Header bar
    auto *headerLayout = new QHBoxLayout;
    m_unreadBadge = new QLabel(QStringLiteral("0 unread"));
    m_unreadBadge->setStyleSheet(
        QStringLiteral("background:#f38ba8;color:#1e1e2e;border-radius:8px;padding:2px 8px;font-weight:bold;"));
    headerLayout->addWidget(m_unreadBadge);
    headerLayout->addStretch();
    m_clearAlertsBtn = new QPushButton(QStringLiteral("Clear All Alerts"));
    connect(m_clearAlertsBtn, &QPushButton::clicked,
            this,             &MainWindow::onClearAlertsClicked);
    headerLayout->addWidget(m_clearAlertsBtn);
    layout->addLayout(headerLayout);

    m_alertList = new QListWidget;
    m_alertList->setAlternatingRowColors(true);
    layout->addWidget(m_alertList);

    connect(m_alertSystem, &AlertSystem::unacknowledgedCountChanged,
            this, &MainWindow::updateUnreadBadge);

    return page;
}

// ── Build settings page ───────────────────────────────────────────────────────

QWidget *MainWindow::buildSettingsPage()
{
    auto *scroll   = new QScrollArea;
    auto *page     = new QWidget;
    auto *layout   = new QVBoxLayout(page);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    // ── AI Provider section ──────────────────────────────────────────────────
    auto *aiGroup  = new QGroupBox(QStringLiteral("AI Provider"));
    auto *aiLayout = new QGridLayout(aiGroup);
    aiLayout->setColumnStretch(1, 1);

    aiLayout->addWidget(new QLabel(QStringLiteral("Provider:")), 0, 0);
    m_providerCombo = new QComboBox;
    m_providerCombo->addItems({
        QStringLiteral("OpenAI (GPT-4o)"),
        QStringLiteral("Google Gemini"),
        QStringLiteral("Anthropic Claude"),
        QStringLiteral("Ollama (Local)"),
        QStringLiteral("Custom Endpoint")
    });
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,            &MainWindow::onProviderChanged);
    aiLayout->addWidget(m_providerCombo, 0, 1);

    aiLayout->addWidget(new QLabel(QStringLiteral("API Key:")), 1, 0);
    m_apiKeyEdit = new QLineEdit;
    m_apiKeyEdit->setPlaceholderText(QStringLiteral("Enter API key…"));
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    aiLayout->addWidget(m_apiKeyEdit, 1, 1);

    aiLayout->addWidget(new QLabel(QStringLiteral("Model:")), 2, 0);
    m_modelEdit = new QLineEdit;
    m_modelEdit->setPlaceholderText(QStringLiteral("e.g. gpt-4o"));
    aiLayout->addWidget(m_modelEdit, 2, 1);

    aiLayout->addWidget(new QLabel(QStringLiteral("Custom URL:")), 3, 0);
    m_customEndpointEdit = new QLineEdit;
    m_customEndpointEdit->setPlaceholderText(
        QStringLiteral("https://your-endpoint/v1/chat/completions"));
    aiLayout->addWidget(m_customEndpointEdit, 3, 1);

    layout->addWidget(aiGroup);

    // ── Monitoring section ───────────────────────────────────────────────────
    auto *monGroup  = new QGroupBox(QStringLiteral("Monitoring"));
    auto *monLayout = new QGridLayout(monGroup);
    monLayout->setColumnStretch(1, 1);

    monLayout->addWidget(new QLabel(QStringLiteral("Interval (ms):")), 0, 0);
    m_intervalSpin = new QSpinBox;
    m_intervalSpin->setRange(250, 60000);
    m_intervalSpin->setSingleStep(250);
    m_intervalSpin->setValue(5000);
    m_intervalSpin->setSuffix(QStringLiteral(" ms"));
    monLayout->addWidget(m_intervalSpin, 0, 1);

    m_externalApiEnabledCheck = new QCheckBox(QStringLiteral("Forward alerts to external API"));
    monLayout->addWidget(m_externalApiEnabledCheck, 1, 0, 1, 2);

    monLayout->addWidget(new QLabel(QStringLiteral("Webhook URL:")), 2, 0);
    m_externalWebhookUrlEdit = new QLineEdit;
    m_externalWebhookUrlEdit->setPlaceholderText(QStringLiteral("https://example.com/security-alerts"));
    monLayout->addWidget(m_externalWebhookUrlEdit, 2, 1);

    monLayout->addWidget(new QLabel(QStringLiteral("Webhook API Key:")), 3, 0);
    m_externalApiKeyEdit = new QLineEdit;
    m_externalApiKeyEdit->setEchoMode(QLineEdit::Password);
    m_externalApiKeyEdit->setPlaceholderText(QStringLiteral("Optional bearer token"));
    monLayout->addWidget(m_externalApiKeyEdit, 3, 1);

    layout->addWidget(monGroup);

    // ── App section ──────────────────────────────────────────────────────────
    auto *appGroup  = new QGroupBox(QStringLiteral("Application"));
    auto *appLayout = new QGridLayout(appGroup);
    appLayout->setColumnStretch(1, 1);

    appLayout->addWidget(new QLabel(QStringLiteral("Language:")), 0, 0);
    m_languageCombo = new QComboBox;
    m_languageCombo->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_languageCombo->addItem(QStringLiteral("العربية"), QStringLiteral("ar"));
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,            &MainWindow::onLanguageChanged);
    appLayout->addWidget(m_languageCombo, 0, 1);

    m_notificationsCheck = new QCheckBox(QStringLiteral("Enable desktop notifications"));
    m_notificationsCheck->setChecked(true);
    appLayout->addWidget(m_notificationsCheck, 1, 0, 1, 2);

    m_loggingCheck = new QCheckBox(QStringLiteral("Enable console logs (no file storage)"));
    m_loggingCheck->setChecked(false);
    appLayout->addWidget(m_loggingCheck, 2, 0, 1, 2);

    layout->addWidget(appGroup);

    // ── Save button ──────────────────────────────────────────────────────────
    m_saveSettingsBtn = new QPushButton(QStringLiteral("✅  Apply Settings"));
    m_saveSettingsBtn->setMinimumHeight(36);
    connect(m_saveSettingsBtn, &QPushButton::clicked,
            this,              &MainWindow::onSettingsSaveClicked);
    layout->addWidget(m_saveSettingsBtn);

    layout->addStretch();

    // Populate from config
    if (m_config) {
        const QString provider = m_config->activeProvider();
        if      (provider == "openai")    m_providerCombo->setCurrentIndex(0);
        else if (provider == "gemini")    m_providerCombo->setCurrentIndex(1);
        else if (provider == "anthropic") m_providerCombo->setCurrentIndex(2);
        else if (provider == "ollama")    m_providerCombo->setCurrentIndex(3);
        else                              m_providerCombo->setCurrentIndex(4);

        m_apiKeyEdit->setText(m_config->apiKey(provider));
        m_modelEdit->setText(m_config->activeModel());
        m_customEndpointEdit->setText(m_config->customEndpoint());
        m_intervalSpin->setValue(m_config->screenshotIntervalMs());
        m_externalApiEnabledCheck->setChecked(m_config->externalApiEnabled());
        m_externalWebhookUrlEdit->setText(m_config->externalApiWebhookUrl());
        m_externalApiKeyEdit->setText(m_config->externalApiKey());

        const QString lang = m_config->language();
        m_languageCombo->setCurrentIndex(lang == "ar" ? 1 : 0);

        m_notificationsCheck->setChecked(m_config->notificationsEnabled());
        m_loggingCheck->setChecked(m_config->loggingEnabled());
    }

    scroll->setWidget(page);
    scroll->setWidgetResizable(true);
    return scroll;
}

// ── Build about page ──────────────────────────────────────────────────────────

QWidget *MainWindow::buildAboutPage()
{
    auto *page   = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel(QStringLiteral(
        "<h1 style='color:#89b4fa;'>IraqiAware</h1>"
        "<h3 style='color:#a6adc8;'>CyberSecurity Awareness Monitorizer</h3>"
        "<h4 style='color:#a6adc8;'>Powered by AI</h4>"));
    title->setAlignment(Qt::AlignCenter);
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    auto *desc = new QLabel(QStringLiteral(
        "<p style='max-width:540px; text-align:center; color:#cdd6f4;'>"
        "IraqiAware monitors your screen in real-time using AI to detect "
        "cybersecurity threats: weak passwords, phishing sites, exposed sensitive "
        "data, and unsafe browsing habits."
        "</p>"
        "<p style='text-align:center; color:#a6adc8;'>"
        "Version 1.0.0 &nbsp;|&nbsp; C++17 / Qt 6 &nbsp;|&nbsp; "
        "<a href='https://github.com/mustafa-cybersecurity/iraqiaware' style='color:#89b4fa;'>"
        "GitHub</a>"
        "</p>"));
    desc->setAlignment(Qt::AlignCenter);
    desc->setTextFormat(Qt::RichText);
    desc->setOpenExternalLinks(true);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    return page;
}

// ── Slots: monitoring control ─────────────────────────────────────────────────

void MainWindow::onStartStopClicked()
{
    if (!m_monitoring) {
        // Validate configuration
        if (!m_aiService->isConfigured()) {
            QMessageBox::warning(this,
                QStringLiteral("Configuration Required"),
                QStringLiteral("Please enter your AI API key in the Settings tab before starting."));
            m_tabs->setCurrentIndex(2); // Settings tab
            return;
        }

        m_monitoring = true;
        m_screenshotMgr->start(m_config ? m_config->screenshotIntervalMs() : 5000);
        m_startStopBtn->setText(QStringLiteral("⏹  Stop Monitoring"));
        m_startStopBtn->setObjectName(QStringLiteral("stopBtn"));
        m_startStopBtn->setStyleSheet(
            QStringLiteral("background-color:#f38ba8;color:#1e1e2e;font-weight:bold;"));
        m_statusLabel->setText(QStringLiteral("Status: ● Monitoring"));
        m_statusLabel->setStyleSheet(QStringLiteral("color:#a6e3a1;font-weight:bold;"));
        updateStatusBar(QStringLiteral("Monitoring started – AI analysis running every %1 ms")
                        .arg(m_config ? m_config->screenshotIntervalMs() : 5000));
    } else {
        m_monitoring = false;
        m_screenshotMgr->stop();
        m_startStopBtn->setText(QStringLiteral("▶  Start Monitoring"));
        m_startStopBtn->setObjectName(QStringLiteral("startBtn"));
        m_startStopBtn->setStyleSheet(
            QStringLiteral("background-color:#a6e3a1;color:#1e1e2e;font-weight:bold;"));
        m_statusLabel->setText(QStringLiteral("Status: Idle"));
        m_statusLabel->setStyleSheet({});
        updateStatusBar(QStringLiteral("Monitoring stopped."));
    }
}

// ── Slots: settings ───────────────────────────────────────────────────────────

void MainWindow::onSettingsSaveClicked()
{
    const QString providerKey = providerName(m_providerCombo->currentIndex());
    const QString apiKey      = m_apiKeyEdit->text().trimmed();
    const QString model       = m_modelEdit->text().trimmed();
    const QString endpoint    = m_customEndpointEdit->text().trimmed();
    const int     interval    = m_intervalSpin->value();
    const QString lang        = m_languageCombo->currentData().toString();
    const bool    externalApiEnabled = m_externalApiEnabledCheck->isChecked();
    const QString externalWebhookUrl = m_externalWebhookUrlEdit->text().trimmed();
    const QString externalApiKey = m_externalApiKeyEdit->text().trimmed();

    // Apply to config
    if (m_config) {
        m_config->setActiveProvider(providerKey);
        m_config->setApiKey(providerKey, apiKey);
        if (!model.isEmpty()) m_config->setActiveModel(model);
        m_config->setCustomEndpoint(endpoint);
        m_config->setScreenshotIntervalMs(interval);
        m_config->setLanguage(lang);
        m_config->setNotificationsEnabled(m_notificationsCheck->isChecked());
        m_config->setLoggingEnabled(m_loggingCheck->isChecked());
        m_config->setExternalApiEnabled(externalApiEnabled);
        m_config->setExternalApiWebhookUrl(externalWebhookUrl);
        m_config->setExternalApiKey(externalApiKey);
        m_config->save();
    }

    // Apply to live AI service
    const AIServiceLayer::Provider p = [&]{
        switch (m_providerCombo->currentIndex()) {
        case 0: return AIServiceLayer::Provider::OpenAI;
        case 1: return AIServiceLayer::Provider::Gemini;
        case 2: return AIServiceLayer::Provider::Anthropic;
        case 3: return AIServiceLayer::Provider::Ollama;
        default: return AIServiceLayer::Provider::Custom;
        }
    }();
    m_aiService->setProvider(p);
    m_aiService->setApiKey(apiKey);
    if (!model.isEmpty()) m_aiService->setModel(model);
    if (!endpoint.isEmpty()) m_aiService->setCustomEndpoint(endpoint);
    m_alertSystem->setNotificationsEnabled(m_notificationsCheck->isChecked());
    m_alertSystem->configureExternalApi(externalApiEnabled, externalWebhookUrl, externalApiKey);

    // Apply language
    if (m_langMgr) {
        m_langMgr->setLanguage(lang);
        retranslateUi();
    }

    updateStatusBar(QStringLiteral("Settings applied for this session only (not saved to disk)."));
}

void MainWindow::onLanguageChanged(int /*index*/)
{
    // Live preview – will be fully applied on Save
}

void MainWindow::onProviderChanged(int index)
{
    // Show/hide custom endpoint field
    const bool isCustom = (index == 4);
    m_customEndpointEdit->setVisible(isCustom || index == 3);

    // Pre-fill model placeholder
    const QStringList defaults = {
        "gpt-4o", "gemini-1.5-pro", "claude-3-5-sonnet-20241022", "llava", ""
    };
    if (index < defaults.size() && !defaults[index].isEmpty()) {
        m_modelEdit->setPlaceholderText(defaults[index]);
    }

    // Load existing API key for this provider
    if (m_config) {
        const QString key = m_config->apiKey(providerName(index));
        m_apiKeyEdit->setText(key);
    }
}

// ── Slots: data flow ──────────────────────────────────────────────────────────

void MainWindow::onScreenshotCaptured(const QByteArray &imageBytes, const QDateTime &timestamp)
{
    Q_UNUSED(timestamp)
    if (m_screenshotPreview) {
        QPixmap pm;
        pm.loadFromData(imageBytes);
        m_screenshotPreview->setPixmap(
            pm.scaled(m_screenshotPreview->size(),
                      Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    m_progressBar->setValue(0);
}

void MainWindow::onAnalysisComplete(const QString &analysisText)
{
    if (m_analysisOutput) {
        m_analysisOutput->append(
            QStringLiteral("\n── %1 ──\n%2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), analysisText));
    }
}

void MainWindow::onThreatsDetected(const std::vector<SecurityAnalyzer::Threat> &threats)
{
    m_totalThreats += static_cast<int>(threats.size());
    if (m_threatCountLabel)
        m_threatCountLabel->setText(
            QStringLiteral("Threats detected: %1").arg(m_totalThreats));
}

void MainWindow::onNoThreatsDetected()
{
    if (m_analysisOutput)
        m_analysisOutput->append(
            QStringLiteral("\n── %1 ── ✅ No threats detected")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void MainWindow::onAlertRaised(const AlertSystem::Alert &alert)
{
    addAlertRow(alert);
    // Switch to Alerts tab for HIGH/CRITICAL
    if (alert.severity >= SecurityAnalyzer::Severity::High) {
        m_tabs->setCurrentIndex(1);
    }
}

void MainWindow::onCaptureError(const QString &errorMessage)
{
    updateStatusBar(QStringLiteral("Capture error: ") + errorMessage);
}

void MainWindow::onAnalysisError(const QString &errorMessage)
{
    if (m_analysisOutput)
        m_analysisOutput->append(
            QStringLiteral("\n⚠ Error: ") + errorMessage);
    updateStatusBar(QStringLiteral("AI error: ") + errorMessage);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void MainWindow::onClearAlertsClicked()
{
    m_alertList->clear();
    m_alertSystem->acknowledgeAll();
}

void MainWindow::onShowWindowRequested()
{
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::updateStatusBar(const QString &message)
{
    statusBar()->showMessage(message, 8000);
}

void MainWindow::addAlertRow(const AlertSystem::Alert &alert)
{
    if (!m_alertList) return;

    const QString sev = SecurityAnalyzer::severityLabel(alert.severity);
    const QString text = QStringLiteral("[%1] [%2] %3")
                         .arg(alert.timestamp.toString("hh:mm:ss"), sev, alert.message);

    auto *item = new QListWidgetItem(text, m_alertList);

    // Colour-code by severity
    switch (alert.severity) {
    case SecurityAnalyzer::Severity::Critical:
        item->setForeground(QColor(QStringLiteral("#f38ba8")));
        break;
    case SecurityAnalyzer::Severity::High:
        item->setForeground(QColor(QStringLiteral("#fab387")));
        break;
    case SecurityAnalyzer::Severity::Medium:
        item->setForeground(QColor(QStringLiteral("#f9e2af")));
        break;
    case SecurityAnalyzer::Severity::Low:
        item->setForeground(QColor(QStringLiteral("#a6e3a1")));
        break;
    default:
        item->setForeground(QColor(QStringLiteral("#a6adc8")));
        break;
    }

    item->setToolTip(alert.recommendation.isEmpty()
                     ? alert.message
                     : QStringLiteral("💡 ") + alert.recommendation);

    m_alertList->scrollToBottom();
}

void MainWindow::updateUnreadBadge(int count)
{
    if (!m_unreadBadge) return;
    m_unreadBadge->setText(QStringLiteral("%1 unread").arg(count));
    m_unreadBadge->setStyleSheet(count > 0
        ? QStringLiteral("background:#f38ba8;color:#1e1e2e;border-radius:8px;padding:2px 8px;font-weight:bold;")
        : QStringLiteral("background:#a6e3a1;color:#1e1e2e;border-radius:8px;padding:2px 8px;"));

    // Update tab badge
    m_tabs->setTabText(1, count > 0
        ? QStringLiteral("🔔  Alerts (%1)").arg(count)
        : QStringLiteral("🔔  Alerts"));
}

QString MainWindow::providerName(int index) const
{
    switch (index) {
    case 0: return QStringLiteral("openai");
    case 1: return QStringLiteral("gemini");
    case 2: return QStringLiteral("anthropic");
    case 3: return QStringLiteral("ollama");
    default: return QStringLiteral("custom");
    }
}

// ── Close event ───────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Minimise to tray instead of quitting
    hide();
    event->ignore();
}
