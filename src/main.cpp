#include "MainWindow.h"
#include "LoggingSystem.h"
#include "ConfigManager.h"
#include "ThreatModel.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QMetaType>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // High-DPI support for Windows
    SetProcessDPIAware();
#endif

    // Enable high-DPI support in Qt
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName("IraqiAware");
    app.setApplicationDisplayName("CyberSecurity Awareness Monitorizer");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("IraqiAware");

    // Working directory: set to application directory so relative paths work
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    // Register custom types for queued connections
    qRegisterMetaType<std::vector<ThreatInfo>>("std::vector<ThreatInfo>");

    // Initialise logging as early as possible
    LoggingSystem::init("logs", "iraqiaware.log",
                        LoggingSystem::Level::Info, 10, 7);
    LOG_INFO("IraqiAware starting up – v1.0.0");

    // Check for system tray support
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        LOG_WARNING("System tray not available on this platform");
    }

    // Keep running when last window is hidden (needed for tray-only mode)
    QApplication::setQuitOnLastWindowClosed(false);

    // Create and show main window
    MainWindow window;
    window.show();

    LOG_INFO("Main window shown – entering event loop");
    int result = app.exec();

    LOG_INFO("Application exiting with code " + std::to_string(result));
    LoggingSystem::shutdown();
    return result;
}
