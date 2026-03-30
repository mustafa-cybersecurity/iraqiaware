#include "MainWindow.h"
#include "ScreenshotManager.h"
#include "AIServiceLayer.h"
#include "SecurityAnalyzer.h"
#include "ConfigManager.h"
#include "LanguageManager.h"
#include "AlertSystem.h"
#include "LoggingSystem.h"

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_config   (std::make_unique<ConfigManager>())
    , m_lang     (std::make_unique<LanguageManager>())
    , m_screenshot(std::make_unique<ScreenshotManager>())
    , m_ai       (std::make_unique<AIServiceLayer>())
    , m_analyzer (std::make_unique<SecurityAnalyzer>())
    , m_alerts   (std::make_unique<AlertSystem>())
{
    m_config->load();
    m_lang->loadLanguage(m_config->getLanguage());

    // Configure AI service from saved config
    m_ai->setProvider(m_config->getAIProvider());
    m_ai->setApiKey(m_config->getAIApiKey());
    m_ai->setModel(m_config->getAIModel());
    m_ai->setTimeout(m_config->getAITimeoutSec());

    // Configure screenshot manager
    m_screenshot->setCaptureInterval(m_config->getCaptureIntervalSec());

    // Wire screenshot → AI → analyzer pipeline
    m_screenshot->setCaptureCallback([this](const std::string& b64,
                                             const std::string& path) {
        // Called on the capture thread; use Qt signals to hop to UI thread
        QMetaObject::invokeMethod(this, "onCaptureReady",
            Qt::QueuedConnection,
            Q_ARG(QString, QString::fromStdString(b64)),
            Q_ARG(QString, QString::fromStdString(path)));
    });

    m_analyzer->setThreatCallback([this](const std::vector<ThreatInfo>& threats) {
        QMetaObject::invokeMethod(this, "onThreatDetected",
            Qt::QueuedConnection,
            Q_ARG(std::vector<ThreatInfo>, threats));
    });

    m_alerts->setAlertCallback([this](const AlertSystem::Alert& alert) {
        // Logging only – UI update happens in onThreatDetected
        LOG_INFO("Alert raised: " + alert.threat.title);
    });

    setupUi();
    loadStyleSheet();
    setupSystemTray();
    setupConnections();
    applyLanguage();

    // Refresh timer – every second
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshDashboard);
    m_refreshTimer->start();

    // Auto-start monitoring if configured
    if (m_config->isMonitoringEnabled()) {
        toggleMonitoring();
    }

    LOG_INFO("MainWindow initialised");
}

MainWindow::~MainWindow() {
    if (m_monitoring) {
        m_screenshot->stop();
    }
}

// ── Close event ───────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event) {
    // Minimise to tray instead of closing
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

// ── UI Setup ──────────────────────────────────────────────────────────────────

void MainWindow::setupUi() {
    setWindowTitle(QString::fromStdString(m_config->getAppName()));
    setMinimumSize(1000, 650);
    resize(1100, 700);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Left navigation panel ────────────────────────────────────────────────
    auto* navPanel = new QWidget;
    navPanel->setObjectName("navPanel");
    navPanel->setFixedWidth(200);
    auto* navLayout = new QVBoxLayout(navPanel);
    navLayout->setContentsMargins(10, 20, 10, 10);
    navLayout->setSpacing(8);

    // Logo / title
    auto* lblLogo = new QLabel("🛡 IraqiAware");
    lblLogo->setObjectName("logoLabel");
    lblLogo->setAlignment(Qt::AlignCenter);
    navLayout->addWidget(lblLogo);

    auto* lblSub = new QLabel("AI Security Monitor");
    lblSub->setObjectName("subLabel");
    lblSub->setAlignment(Qt::AlignCenter);
    navLayout->addWidget(lblSub);
    navLayout->addSpacing(20);

    m_btnNavDashboard = new QPushButton("📊  Dashboard");
    m_btnNavAlerts    = new QPushButton("🚨  Alerts");
    m_btnNavSettings  = new QPushButton("⚙  Settings");
    m_btnToggleMon    = new QPushButton("▶  Start Monitoring");

    for (auto* btn : {m_btnNavDashboard, m_btnNavAlerts, m_btnNavSettings}) {
        btn->setObjectName("navButton");
        btn->setCheckable(true);
        btn->setMinimumHeight(40);
        navLayout->addWidget(btn);
    }

    navLayout->addStretch();
    m_btnToggleMon->setObjectName("toggleButton");
    m_btnToggleMon->setMinimumHeight(44);
    navLayout->addWidget(m_btnToggleMon);
    navLayout->addSpacing(10);

    // ── Stacked content area ─────────────────────────────────────────────────
    m_stack = new QStackedWidget;
    m_stack->setObjectName("contentStack");
    setupDashboardPage();
    setupAlertsPage();
    setupSettingsPage();

    mainLayout->addWidget(navPanel);
    mainLayout->addWidget(m_stack, 1);

    // Status bar
    statusBar()->showMessage(tr("Ready"));

    m_btnNavDashboard->setChecked(true);
}

void MainWindow::setupDashboardPage() {
    auto* page = new QWidget;
    auto* vl   = new QVBoxLayout(page);
    vl->setContentsMargins(20, 20, 20, 20);
    vl->setSpacing(12);

    auto* title = new QLabel(tr("Dashboard"));
    title->setObjectName("pageTitle");
    vl->addWidget(title);

    // Stats row
    auto* statsRow = new QHBoxLayout;
    statsRow->setSpacing(12);

    auto makeStatCard = [&](const QString& label, QLabel*& valueRef) {
        auto* card = new QWidget;
        card->setObjectName("statCard");
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(16, 12, 16, 12);
        auto* lbl = new QLabel(label);
        lbl->setObjectName("statLabel");
        valueRef = new QLabel("—");
        valueRef->setObjectName("statValue");
        cl->addWidget(lbl);
        cl->addWidget(valueRef);
        return card;
    };

    statsRow->addWidget(makeStatCard("Monitoring Status",  m_lblStatus));
    statsRow->addWidget(makeStatCard("Active Threats",     m_lblThreatCount));
    statsRow->addWidget(makeStatCard("Last Scan",          m_lblLastScan));
    statsRow->addWidget(makeStatCard("Screenshots Taken",  m_lblScreenshots));
    vl->addLayout(statsRow);

    // Log output
    auto* logGroup = new QGroupBox(tr("Activity Log"));
    logGroup->setObjectName("contentGroup");
    auto* logLayout = new QVBoxLayout(logGroup);
    m_logOutput = new QTextEdit;
    m_logOutput->setReadOnly(true);
    m_logOutput->setObjectName("logOutput");
    m_logOutput->setPlaceholderText("Application events will appear here...");
    logLayout->addWidget(m_logOutput);
    vl->addWidget(logGroup, 1);

    m_stack->addWidget(page);
}

void MainWindow::setupAlertsPage() {
    auto* page = new QWidget;
    auto* vl   = new QVBoxLayout(page);
    vl->setContentsMargins(20, 20, 20, 20);
    vl->setSpacing(12);

    auto* title = new QLabel(tr("Security Alerts"));
    title->setObjectName("pageTitle");
    vl->addWidget(title);

    // Toolbar
    auto* toolbar = new QHBoxLayout;
    m_btnExportJson  = new QPushButton("Export JSON");
    m_btnExportCsv   = new QPushButton("Export CSV");
    m_btnClearAlerts = new QPushButton("Clear All");
    for (auto* btn : {m_btnExportJson, m_btnExportCsv, m_btnClearAlerts}) {
        btn->setObjectName("toolButton");
        toolbar->addWidget(btn);
    }
    toolbar->addStretch();
    vl->addLayout(toolbar);

    // Alerts table
    m_alertsTable = new QTableWidget(0, 5);
    m_alertsTable->setObjectName("alertsTable");
    m_alertsTable->setHorizontalHeaderLabels(
        {"Severity", "Category", "Title", "Description", "Time"});
    m_alertsTable->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::Stretch);
    m_alertsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_alertsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_alertsTable->verticalHeader()->setVisible(false);
    vl->addWidget(m_alertsTable, 1);

    m_stack->addWidget(page);
}

void MainWindow::setupSettingsPage() {
    auto* page   = new QWidget;
    auto* scroll = new QScrollArea;
    scroll->setWidget(page);
    scroll->setWidgetResizable(true);
    scroll->setObjectName("settingsScroll");

    auto* vl = new QVBoxLayout(page);
    vl->setContentsMargins(20, 20, 20, 20);
    vl->setSpacing(16);

    auto* title = new QLabel(tr("Settings"));
    title->setObjectName("pageTitle");
    vl->addWidget(title);

    // ── General ──────────────────────────────────────────────────────────────
    auto* genGroup = new QGroupBox("General");
    genGroup->setObjectName("contentGroup");
    auto* genLayout = new QVBoxLayout(genGroup);

    auto* langRow = new QHBoxLayout;
    langRow->addWidget(new QLabel("Language:"));
    m_cmbLanguage = new QComboBox;
    m_cmbLanguage->addItem("English", "en");
    m_cmbLanguage->addItem("العربية", "ar");
    langRow->addWidget(m_cmbLanguage);
    langRow->addStretch();
    genLayout->addLayout(langRow);

    m_chkMonitoring = new QCheckBox("Enable monitoring on startup");
    m_chkMonitoring->setChecked(m_config->isMonitoringEnabled());
    genLayout->addWidget(m_chkMonitoring);

    auto* intervalRow = new QHBoxLayout;
    intervalRow->addWidget(new QLabel("Capture interval (seconds):"));
    m_spnInterval = new QSpinBox;
    m_spnInterval->setRange(1, 300);
    m_spnInterval->setValue(m_config->getCaptureIntervalSec());
    intervalRow->addWidget(m_spnInterval);
    intervalRow->addStretch();
    genLayout->addLayout(intervalRow);

    vl->addWidget(genGroup);

    // ── AI Provider ──────────────────────────────────────────────────────────
    auto* aiGroup = new QGroupBox("AI Provider");
    aiGroup->setObjectName("contentGroup");
    auto* aiLayout = new QVBoxLayout(aiGroup);

    auto* providerRow = new QHBoxLayout;
    providerRow->addWidget(new QLabel("Provider:"));
    m_cmbProvider = new QComboBox;
    m_cmbProvider->addItem("OpenAI GPT-4 Vision", "openai");
    m_cmbProvider->addItem("Google Cloud Vision", "google");
    int pIdx = m_cmbProvider->findData(
        QString::fromStdString(m_config->getAIProvider()));
    if (pIdx >= 0) m_cmbProvider->setCurrentIndex(pIdx);
    providerRow->addWidget(m_cmbProvider);
    providerRow->addStretch();
    aiLayout->addLayout(providerRow);

    auto* keyRow = new QHBoxLayout;
    keyRow->addWidget(new QLabel("API Key:"));
    m_edtApiKey = new QLineEdit;
    m_edtApiKey->setEchoMode(QLineEdit::Password);
    m_edtApiKey->setPlaceholderText("Enter your API key...");
    m_edtApiKey->setText(QString::fromStdString(m_config->getAIApiKey()));
    keyRow->addWidget(m_edtApiKey, 1);
    aiLayout->addLayout(keyRow);

    auto* modelRow = new QHBoxLayout;
    modelRow->addWidget(new QLabel("Model:"));
    m_cmbModel = new QComboBox;
    m_cmbModel->addItem("gpt-4o");
    m_cmbModel->addItem("gpt-4-vision-preview");
    m_cmbModel->addItem("gpt-4-turbo");
    int mIdx = m_cmbModel->findText(
        QString::fromStdString(m_config->getAIModel()));
    if (mIdx >= 0) m_cmbModel->setCurrentIndex(mIdx);
    modelRow->addWidget(m_cmbModel);
    modelRow->addStretch();
    aiLayout->addLayout(modelRow);

    m_btnTestApi = new QPushButton("Test Connection");
    m_btnTestApi->setObjectName("toolButton");
    aiLayout->addWidget(m_btnTestApi);
    vl->addWidget(aiGroup);

    // ── Alerts ───────────────────────────────────────────────────────────────
    auto* alertGroup = new QGroupBox("Alerts");
    alertGroup->setObjectName("contentGroup");
    auto* alertLayout = new QVBoxLayout(alertGroup);
    m_chkAlerts = new QCheckBox("Enable notifications");
    m_chkAlerts->setChecked(m_config->isAlertsEnabled());
    m_chkSound  = new QCheckBox("Enable sound alerts");
    m_chkSound->setChecked(m_config->isSoundEnabled());
    alertLayout->addWidget(m_chkAlerts);
    alertLayout->addWidget(m_chkSound);
    vl->addWidget(alertGroup);

    // ── Save button ───────────────────────────────────────────────────────────
    m_btnSaveSettings = new QPushButton("💾  Save Settings");
    m_btnSaveSettings->setObjectName("primaryButton");
    m_btnSaveSettings->setMinimumHeight(40);
    vl->addWidget(m_btnSaveSettings);
    vl->addStretch();

    m_stack->addWidget(scroll);
}

void MainWindow::setupSystemTray() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip("IraqiAware – CyberSecurity Monitor");

    auto* menu = new QMenu(this);
    menu->addAction("Show",  this, &MainWindow::showFromTray);
    menu->addAction("Dashboard", this, &MainWindow::showDashboard);
    menu->addSeparator();
    menu->addAction("Exit", qApp, &QApplication::quit);
    m_trayIcon->setContextMenu(menu);

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayActivated);
    m_trayIcon->show();
}

void MainWindow::setupConnections() {
    connect(m_btnNavDashboard, &QPushButton::clicked, this, &MainWindow::showDashboard);
    connect(m_btnNavAlerts,    &QPushButton::clicked, this, &MainWindow::showAlerts);
    connect(m_btnNavSettings,  &QPushButton::clicked, this, &MainWindow::showSettings);
    connect(m_btnToggleMon,    &QPushButton::clicked, this, &MainWindow::toggleMonitoring);
    connect(m_btnSaveSettings, &QPushButton::clicked, this, &MainWindow::saveSettings);
    connect(m_btnTestApi,      &QPushButton::clicked, this, &MainWindow::testApiConnection);
    connect(m_cmbLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLanguageChanged);
    connect(m_cmbProvider, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProviderChanged);

    connect(m_btnExportJson, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this, "Export JSON", "alerts.json", "JSON files (*.json)");
        if (!path.isEmpty()) {
            QFile f(path);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(QByteArray::fromStdString(m_alerts->exportAlertsJson()));
            }
        }
    });

    connect(m_btnExportCsv, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this, "Export CSV", "threats.csv", "CSV files (*.csv)");
        if (!path.isEmpty()) {
            QFile f(path);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(QByteArray::fromStdString(m_analyzer->exportHistoryCsv()));
            }
        }
    });

    connect(m_btnClearAlerts, &QPushButton::clicked, this, [this]() {
        m_alerts->dismissAll();
        m_alertsTable->setRowCount(0);
        m_logOutput->append("<i>All alerts cleared.</i>");
    });
}

// ── Navigation ────────────────────────────────────────────────────────────────

void MainWindow::showDashboard() {
    m_stack->setCurrentIndex(0);
    m_btnNavDashboard->setChecked(true);
    m_btnNavAlerts->setChecked(false);
    m_btnNavSettings->setChecked(false);
}

void MainWindow::showAlerts() {
    m_stack->setCurrentIndex(1);
    m_btnNavAlerts->setChecked(true);
    m_btnNavDashboard->setChecked(false);
    m_btnNavSettings->setChecked(false);
}

void MainWindow::showSettings() {
    m_stack->setCurrentIndex(2);
    m_btnNavSettings->setChecked(true);
    m_btnNavDashboard->setChecked(false);
    m_btnNavAlerts->setChecked(false);
}

// ── Monitoring ────────────────────────────────────────────────────────────────

void MainWindow::toggleMonitoring() {
    if (!m_monitoring) {
        if (!m_ai->isConfigured()) {
            m_logOutput->append(
                "<span style='color:#e74c3c'>⚠ AI service not configured. "
                "Please set your API key in Settings.</span>");
            showSettings();
            return;
        }
        m_screenshot->start();
        m_monitoring = true;
        m_btnToggleMon->setText("⏹  Stop Monitoring");
        m_btnToggleMon->setProperty("active", true);
        m_btnToggleMon->style()->unpolish(m_btnToggleMon);
        m_btnToggleMon->style()->polish(m_btnToggleMon);
        m_lblStatus->setText("<span style='color:#2ecc71'>● Active</span>");
        m_logOutput->append("<b>Monitoring started.</b>");
        statusBar()->showMessage("Monitoring active");
        LOG_INFO("Monitoring started by user");
    } else {
        m_screenshot->stop();
        m_monitoring = false;
        m_btnToggleMon->setText("▶  Start Monitoring");
        m_btnToggleMon->setProperty("active", false);
        m_btnToggleMon->style()->unpolish(m_btnToggleMon);
        m_btnToggleMon->style()->polish(m_btnToggleMon);
        m_lblStatus->setText("<span style='color:#e74c3c'>● Stopped</span>");
        m_logOutput->append("<b>Monitoring stopped.</b>");
        statusBar()->showMessage("Monitoring stopped");
        LOG_INFO("Monitoring stopped by user");
    }
}

// ── Screenshot → AI pipeline ──────────────────────────────────────────────────

void MainWindow::onCaptureReady(const QString& base64, const QString& filePath) {
    m_logOutput->append("📷 Screenshot captured → analysing with AI...");

    // Analyse on the Qt thread (quick; cURL handles its own I/O)
    m_ai->analyzeScreenshot(
        base64.toStdString(),
        [this, filePath](bool ok, const std::string& response, const std::string& err) {
            QMetaObject::invokeMethod(this, "onAnalysisComplete",
                Qt::QueuedConnection,
                Q_ARG(bool,    ok),
                Q_ARG(QString, QString::fromStdString(response)),
                Q_ARG(QString, QString::fromStdString(err)));
        });
}

void MainWindow::onAnalysisComplete(bool success,
                                     const QString& response,
                                     const QString& error) {
    if (!success) {
        m_logOutput->append(
            "<span style='color:#e74c3c'>✗ AI analysis failed: " +
            error.toHtmlEscaped() + "</span>");
        return;
    }

    auto result = m_analyzer->parseAIResponse(
        response.toStdString(),
        m_screenshot->getLastScreenshotPath());

    if (result.threats.empty()) {
        m_logOutput->append(
            "<span style='color:#2ecc71'>✓ No threats detected.</span>");
    } else {
        m_logOutput->append(
            "<span style='color:#e67e22'>⚠ " +
            QString::number(result.threats.size()) +
            " threat(s) detected!</span>");
    }
}

void MainWindow::onThreatDetected(const std::vector<ThreatInfo>& threats) {
    m_alerts->raiseAlerts(threats);
    for (auto& t : threats) {
        addAlertRow(t);
    }
    updateDashboardStats();
}

// ── Dashboard refresh ─────────────────────────────────────────────────────────

void MainWindow::refreshDashboard() {
    updateDashboardStats();
}

void MainWindow::updateDashboardStats() {
    auto threats = m_analyzer->getAllThreats();

    m_lblThreatCount->setText(QString::number(threats.size()));
    m_lblScreenshots->setText(
        QString::number(m_screenshot->getCaptureCount()));

    auto lastPath = m_screenshot->getLastScreenshotPath();
    if (!lastPath.empty()) {
        auto now = QDateTime::currentDateTime();
        m_lblLastScan->setText(now.toString("hh:mm:ss"));
    }
}

// ── Alerts table ──────────────────────────────────────────────────────────────

void MainWindow::addAlertRow(const ThreatInfo& threat) {
    int row = m_alertsTable->rowCount();
    m_alertsTable->insertRow(row);

    // Severity cell (coloured)
    auto* sevItem = new QTableWidgetItem(
        QString::fromStdString(ThreatInfo::severityToString(threat.severity)));
    QColor sevColor;
    switch (threat.severity) {
        case ThreatSeverity::Critical: sevColor = QColor("#e74c3c"); break;
        case ThreatSeverity::High:     sevColor = QColor("#e67e22"); break;
        case ThreatSeverity::Medium:   sevColor = QColor("#f1c40f"); break;
        case ThreatSeverity::Low:      sevColor = QColor("#2ecc71"); break;
    }
    sevItem->setForeground(sevColor);
    m_alertsTable->setItem(row, 0, sevItem);
    m_alertsTable->setItem(row, 1, new QTableWidgetItem(
        QString::fromStdString(ThreatInfo::categoryToString(threat.category))));
    m_alertsTable->setItem(row, 2, new QTableWidgetItem(
        QString::fromStdString(threat.title)));
    m_alertsTable->setItem(row, 3, new QTableWidgetItem(
        QString::fromStdString(threat.description)));
    m_alertsTable->setItem(row, 4, new QTableWidgetItem(
        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));

    m_alertsTable->scrollToBottom();
}

// ── Settings ──────────────────────────────────────────────────────────────────

void MainWindow::saveSettings() {
    m_config->setLanguage(m_cmbLanguage->currentData().toString().toStdString());
    m_config->setAIProvider(m_cmbProvider->currentData().toString().toStdString());
    m_config->setAIApiKey(m_edtApiKey->text().toStdString());
    m_config->setAIModel(m_cmbModel->currentText().toStdString());
    m_config->setCaptureIntervalSec(m_spnInterval->value());
    m_config->setMonitoringEnabled(m_chkMonitoring->isChecked());
    m_config->setAlertsEnabled(m_chkAlerts->isChecked());
    m_config->setSoundEnabled(m_chkSound->isChecked());

    if (m_config->save()) {
        // Apply immediately
        m_ai->setProvider(m_config->getAIProvider());
        m_ai->setApiKey(m_config->getAIApiKey());
        m_ai->setModel(m_config->getAIModel());
        m_screenshot->setCaptureInterval(m_config->getCaptureIntervalSec());
        m_alerts->setSoundEnabled(m_config->isSoundEnabled());
        m_alerts->setNotificationsEnabled(m_config->isAlertsEnabled());

        applyLanguage();

        QMessageBox::information(this, "Settings", "Settings saved successfully.");
        LOG_INFO("Settings saved");
    } else {
        QMessageBox::warning(this, "Settings", "Failed to save settings.");
    }
}

void MainWindow::testApiConnection() {
    QString key = m_edtApiKey->text().trimmed();
    if (key.isEmpty()) {
        QMessageBox::warning(this, "Test Connection",
            "Please enter an API key first.");
        return;
    }

    m_ai->setProvider(m_cmbProvider->currentData().toString().toStdString());
    m_ai->setApiKey(key.toStdString());
    m_ai->setModel(m_cmbModel->currentText().toStdString());

    statusBar()->showMessage("Testing API connection...");

    // Send a minimal request with a blank image placeholder
    m_ai->analyzeScreenshot("", [this](bool ok,
                                        const std::string& /*resp*/,
                                        const std::string& err) {
        QMetaObject::invokeMethod(this, [this, ok, err = QString::fromStdString(err)]() {
            if (ok) {
                QMessageBox::information(this, "Test Connection",
                    "✓ API connection successful!");
                statusBar()->showMessage("API test passed");
            } else {
                QMessageBox::warning(this, "Test Connection",
                    "✗ Connection failed:\n" + err);
                statusBar()->showMessage("API test failed");
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onLanguageChanged(int /*index*/) {
    QString langCode = m_cmbLanguage->currentData().toString();
    m_lang->loadLanguage(langCode.toStdString());
    applyLanguage();
}

void MainWindow::onProviderChanged(int /*index*/) {
    QString prov = m_cmbProvider->currentData().toString();
    // Adjust default model list per provider
    m_cmbModel->clear();
    if (prov == "openai") {
        m_cmbModel->addItems({"gpt-4o", "gpt-4-vision-preview", "gpt-4-turbo"});
    } else {
        m_cmbModel->addItem("default");
    }
}

// ── System tray ───────────────────────────────────────────────────────────────

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick) {
        showFromTray();
    }
}

void MainWindow::showFromTray() {
    showNormal();
    activateWindow();
    raise();
}

// ── Styling & localisation ────────────────────────────────────────────────────

void MainWindow::loadStyleSheet() {
    QFile qss("resources/styles/dark_theme.qss");
    if (qss.open(QFile::ReadOnly)) {
        setStyleSheet(QString(qss.readAll()));
    } else {
        // Inline minimal dark theme fallback
        setStyleSheet(R"(
            QMainWindow, QWidget { background: #1e1e2e; color: #cdd6f4; }
            QPushButton { background: #313244; border: 1px solid #45475a;
                          padding: 6px 12px; border-radius: 4px; }
            QPushButton:hover { background: #45475a; }
            QLineEdit, QComboBox, QSpinBox { background: #313244;
                border: 1px solid #45475a; padding: 4px; border-radius: 4px; }
            QTableWidget { gridline-color: #313244; }
            QHeaderView::section { background: #313244; padding: 4px; }
        )");
    }
}

void MainWindow::applyLanguage() {
    // Re-translate UI elements
    setWindowTitle(QString::fromStdString(m_lang->tr("app.title")));
    if (m_lang->isRTL()) {
        setLayoutDirection(Qt::RightToLeft);
    } else {
        setLayoutDirection(Qt::LeftToRight);
    }
}
