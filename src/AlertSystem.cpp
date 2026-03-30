#include "AlertSystem.h"
#include "LoggingSystem.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <shellapi.h>
#  pragma comment(lib, "shell32.lib")
#endif

using json = nlohmann::json;

namespace {
std::string generateAlertId() {
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream ss;
    ss << "alert-" << std::hex << std::setw(12) << std::setfill('0') << dist(rng);
    return ss.str();
}
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

AlertSystem::AlertSystem() = default;

AlertSystem::~AlertSystem() {
    cleanup();
}

bool AlertSystem::initTrayIcon(void* hwnd) {
#ifdef _WIN32
    m_hwnd = hwnd;

    auto* nid = new NOTIFYICONDATA{};
    nid->cbSize           = sizeof(NOTIFYICONDATA);
    nid->hWnd             = static_cast<HWND>(hwnd);
    nid->uID              = 1;
    nid->uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid->uCallbackMessage = WM_USER + 1;
    nid->hIcon            = LoadIcon(nullptr, IDI_SHIELD);
    wcscpy_s(nid->szTip, L"IraqiAware - CyberSecurity Monitor");
    Shell_NotifyIcon(NIM_ADD, nid);
    m_nid             = nid;
    m_trayInitialised = true;
    LOG_INFO("System tray icon initialised");
    return true;
#else
    (void)hwnd;
    return false;
#endif
}

void AlertSystem::cleanup() {
#ifdef _WIN32
    if (m_trayInitialised && m_nid) {
        Shell_NotifyIcon(NIM_DELETE, static_cast<NOTIFYICONDATA*>(m_nid));
        delete static_cast<NOTIFYICONDATA*>(m_nid);
        m_nid             = nullptr;
        m_trayInitialised = false;
    }
#endif
}

// ── Raise alert ───────────────────────────────────────────────────────────────

void AlertSystem::raiseAlert(const ThreatInfo& threat) {
    Alert alert;
    alert.id        = generateAlertId();
    alert.threat    = threat;
    alert.createdAt = std::chrono::system_clock::now();

    m_alerts.push_back(alert);
    LOG_WARNING("Alert raised: [" + ThreatInfo::severityToString(threat.severity) +
                "] " + threat.title);

    if (m_notifEnabled) {
        showTrayBalloon(threat.title, threat.description, threat.severity);
    }
    if (m_soundEnabled) {
        playAlertSound(threat.severity);
    }
    if (m_callback) {
        m_callback(alert);
    }
}

void AlertSystem::raiseAlerts(const std::vector<ThreatInfo>& threats) {
    for (auto& t : threats) raiseAlert(t);
}

// ── Dismiss ───────────────────────────────────────────────────────────────────

void AlertSystem::dismissAlert(const std::string& alertId) {
    for (auto& a : m_alerts) {
        if (a.id == alertId) { a.dismissed = true; break; }
    }
}

void AlertSystem::dismissAll() {
    for (auto& a : m_alerts) a.dismissed = true;
}

std::vector<AlertSystem::Alert> AlertSystem::getActiveAlerts() const {
    std::vector<Alert> active;
    for (auto& a : m_alerts) {
        if (!a.dismissed) active.push_back(a);
    }
    return active;
}

// ── Export ────────────────────────────────────────────────────────────────────

std::string AlertSystem::exportAlertsJson() const {
    json arr = json::array();
    for (auto& a : m_alerts) {
        arr.push_back({
            {"id",          a.id},
            {"dismissed",   a.dismissed},
            {"threat", {
                {"category",       ThreatInfo::categoryToString(a.threat.category)},
                {"severity",       ThreatInfo::severityToString(a.threat.severity)},
                {"title",          a.threat.title},
                {"description",    a.threat.description},
                {"recommendation", a.threat.recommendation}
            }}
        });
    }
    return arr.dump(2);
}

// ── Platform-specific ─────────────────────────────────────────────────────────

void AlertSystem::playAlertSound(ThreatSeverity severity) {
#ifdef _WIN32
    switch (severity) {
        case ThreatSeverity::Critical:
            MessageBeep(MB_ICONERROR);
            break;
        case ThreatSeverity::High:
            MessageBeep(MB_ICONWARNING);
            break;
        default:
            MessageBeep(MB_ICONINFORMATION);
            break;
    }
#else
    (void)severity;
#endif
}

void AlertSystem::showTrayBalloon(const std::string& title,
                                   const std::string& msg,
                                   ThreatSeverity     severity) {
#ifdef _WIN32
    if (!m_trayInitialised || !m_nid) return;

    auto* nid = static_cast<NOTIFYICONDATA*>(m_nid);
    nid->uFlags      |= NIF_INFO;
    nid->dwInfoFlags  = (severity >= ThreatSeverity::High)
                        ? NIIF_ERROR : NIIF_WARNING;
    nid->uTimeout     = 5000;

    // Convert std::string to wchar_t
    auto toWide = [](const std::string& s, wchar_t* out, int maxLen) {
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out, maxLen);
    };
    toWide(title, nid->szInfoTitle, 64);
    toWide(msg,   nid->szInfo,      256);

    Shell_NotifyIcon(NIM_MODIFY, nid);
#else
    (void)title; (void)msg; (void)severity;
#endif
}
