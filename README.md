# IraqiAware – CyberSecurity Awareness Monitorizer
# Powered by AI

## Overview

**IraqiAware** is a Windows desktop application that continuously monitors your screen for cybersecurity threats using AI vision models.

### Key Features

- 🔍 **Automatic Screen Monitoring** – captures a screenshot every 5 seconds (configurable)
- 🤖 **AI-Powered Analysis** – sends screenshots to OpenAI GPT-4 Vision / Google Cloud Vision for threat analysis
- 🚨 **Real-time Threat Detection** – detects weak passwords, phishing sites, exposed sensitive data, and unsafe browsing
- 🌙 **Dark Theme UI** – modern, professional dark interface
- 🌐 **Bilingual** – full English and Arabic (RTL) support
- 📊 **Dashboard** – live monitoring status, threat count, activity log
- ⚙️ **Settings Panel** – configure AI provider, API key, language, and capture frequency
- 🔔 **System Tray Notifications** – Windows balloon tip alerts for detected threats
- 📁 **Export** – export alert history as JSON or CSV

---

## Project Structure

```
iraqiaware/
├── CMakeLists.txt                  # Main CMake build configuration
├── cmake/
│   ├── CompilerSettings.cmake      # MSVC / GCC / Clang flags
│   └── FindQt6.cmake               # Qt6 location hints
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── MainWindow.cpp              # Qt main window (dark theme UI)
│   ├── ScreenshotManager.cpp       # GDI+ screenshot capture (threaded)
│   ├── AIServiceLayer.cpp          # REST API client for AI providers
│   ├── SecurityAnalyzer.cpp        # Parse AI responses → ThreatInfo
│   ├── ConfigManager.cpp           # JSON-based configuration
│   ├── LanguageManager.cpp         # EN/AR translation loader
│   ├── AlertSystem.cpp             # Windows tray notifications
│   └── LoggingSystem.cpp           # spdlog-based rotating file log
├── include/                        # Header files (one per class)
├── resources/
│   ├── styles/dark_theme.qss       # Qt stylesheet (dark Catppuccin theme)
│   └── translations/
│       ├── en.json                 # English strings
│       └── ar.json                 # Arabic strings (RTL)
├── config/
│   ├── default_config.json         # Default app configuration
│   └── api_providers.json          # Supported AI provider definitions
├── vcpkg.json                      # vcpkg dependency manifest
└── README.md
```

---

## Prerequisites

| Requirement | Version |
|---|---|
| Windows | 10 / 11 (x64) |
| Visual Studio | 2022 (with "Desktop development with C++" workload) |
| CMake | 3.16 or newer |
| Qt | 6.x (tested with 6.5+) |
| vcpkg | latest |

---

## Build Instructions

### 1. Install prerequisites

```powershell
# Install vcpkg (if not already installed)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
setx VCPKG_ROOT C:\vcpkg

# Install Qt 6 via Qt Online Installer (https://www.qt.io/download)
# Select: Qt 6.x → MSVC 2022 64-bit
```

### 2. Clone and build

```powershell
git clone https://github.com/mustafa-cybersecurity/iraqiaware.git
cd iraqiaware

mkdir build
cd build

# Configure (Release x64)
cmake -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
      ..

# Build
cmake --build . --config Release --parallel
```

### 3. Run

```powershell
.\build\x64\Release\iraqiaware.exe
```

---

## Configuration

On first launch, open **Settings** and enter:

| Field | Description |
|---|---|
| **AI Provider** | `openai` or `google` |
| **API Key** | Your API key (stored obfuscated on disk) |
| **Model** | `gpt-4o` (recommended), `gpt-4-vision-preview`, etc. |
| **Capture Interval** | Seconds between screenshots (default: 5) |

---

## Threat Categories

| Category | Description |
|---|---|
| **Weak Password** | Visible password fields with easily guessable values |
| **Phishing Detected** | Suspicious URLs, fake login pages, warning banners |
| **Sensitive Data Exposed** | Credit card numbers, SSNs, private keys visible on screen |
| **Unsafe Browsing** | HTTP sites handling sensitive data |
| **Malware Indicator** | Suspicious pop-ups, download prompts |

Severity levels: **Critical** → **High** → **Medium** → **Low**

---

## Technologies

- **C++17** – modern C++ standard
- **Qt 6** – cross-platform GUI framework (dark theme)
- **cURL** – HTTP client for AI API requests
- **nlohmann/json** – JSON parsing
- **spdlog** – fast rotating file logging
- **GDI+** (Windows) – screenshot capture
- **OpenSSL** – TLS for secure API connections

---

## License

MIT License – see [LICENSE](LICENSE)

---

## Arabic Documentation

See [README_AR.md](README_AR.md) for Arabic documentation.