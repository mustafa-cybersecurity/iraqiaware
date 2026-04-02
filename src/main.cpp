#include "MainWindow.h"
#include "ScreenshotManager.h"
#include "AIServiceLayer.h"
#include "SecurityAnalyzer.h"
#include "ConfigManager.h"
#include "LanguageManager.h"
#include "AlertSystem.h"

#include <QApplication>
#include <QDebug>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

/**
 * @brief Initialises the spdlog logging infrastructure.
 *
 * Creates a console-only logger (no persistent file logging).
 */
static void initLogging(bool /*enableFileLog*/)
{
    try {
        std::vector<spdlog::sink_ptr> sinks;

        // Console sink (coloured)
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        auto logger = std::make_shared<spdlog::logger>("iraqiaware",
                                                        sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::warn);
    } catch (const spdlog::spdlog_ex &ex) {
        qWarning() << "Logging init failed:" << ex.what();
    }
}

int main(int argc, char *argv[])
{
    // ── Qt application ────────────────────────────────────────────────────────
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("IraqiAware"));
    app.setOrganizationName(QStringLiteral("MustafaCybersecurity"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    // ── Config ────────────────────────────────────────────────────────────────
    ConfigManager config;
    config.load();

    // ── Logging ───────────────────────────────────────────────────────────────
    initLogging(false);
    spdlog::info("IraqiAware starting up (v{})", app.applicationVersion().toStdString());

    // ── Language ──────────────────────────────────────────────────────────────
    LanguageManager langMgr;
    const QString translationsDir =
        QApplication::applicationDirPath() + QStringLiteral("/resources/translations");
    if (!langMgr.load(translationsDir)) {
        spdlog::warn("Translation files not found in {}", translationsDir.toStdString());
    }
    langMgr.setLanguage(config.language());

    // ── Core components ───────────────────────────────────────────────────────
    ScreenshotManager screenshotMgr;
    AIServiceLayer    aiService;
    SecurityAnalyzer  analyzer;
    AlertSystem       alertSystem;

    // Apply saved AI config
    {
        const QString provider = config.activeProvider();
        const AIServiceLayer::Provider p = [&]{
            if      (provider == "openai")    return AIServiceLayer::Provider::OpenAI;
            else if (provider == "gemini")    return AIServiceLayer::Provider::Gemini;
            else if (provider == "anthropic") return AIServiceLayer::Provider::Anthropic;
            else if (provider == "ollama")    return AIServiceLayer::Provider::Ollama;
            else                              return AIServiceLayer::Provider::Custom;
        }();
        aiService.setProvider(p);
        aiService.setApiKey(config.apiKey(provider));
        aiService.setModel(config.activeModel());
        if (!config.customEndpoint().isEmpty())
            aiService.setCustomEndpoint(config.customEndpoint());
    }

    // ── Alert system tray ────────────────────────────────────────────────────
    alertSystem.setNotificationsEnabled(config.notificationsEnabled());
    alertSystem.configureExternalApi(
        config.externalApiEnabled(),
        config.externalApiWebhookUrl(),
        config.externalApiKey());
    alertSystem.initialise();

    // ── Main window ───────────────────────────────────────────────────────────
    MainWindow window(&screenshotMgr, &aiService, &analyzer,
                      &config, &langMgr, &alertSystem);
    window.show();

    spdlog::info("Main window shown");

    const int ret = app.exec();
    spdlog::info("Application exiting with code {}", ret);
    return ret;
}
