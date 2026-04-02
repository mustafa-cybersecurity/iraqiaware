# IraqiAware – CyberSecurity Awareness Monitorizer

> **Powered by AI** | C++17 | Qt 6 | Windows Desktop

A real-time cybersecurity awareness desktop application that captures your
screen every 5 seconds, sends the image to an AI model of your choice, and
surfaces actionable security alerts – all within a sleek dark-themed GUI with
English and Arabic support.

---

## ✨ Features

| Feature | Details |
|---|---|
| 🖥 **Screen Monitoring** | Automatic screenshot capture every 5 s (configurable) |
| 🤖 **AI-Powered Analysis** | Supports OpenAI GPT-4o, Google Gemini, Anthropic Claude, local Ollama, and any custom OpenAI-compatible endpoint |
| 🔐 **Threat Detection** | Weak passwords · Phishing sites · Exposed sensitive data · Unsafe browsing habits · Any AI-identified risk |
| 🌑 **Dark Mode UI** | Catppuccin Mocha palette via Qt 6 Widgets + custom QSS stylesheet |
| 🌐 **Bilingual** | Full English ↔ Arabic (RTL) UI localization |
| 🔔 **Real-time Alerts** | In-app alert log + Windows system-tray notifications |
| 🌐 **External Alert API** | Optional webhook forwarding for each detected threat |
| 📝 **Logging** | Rotating log files via spdlog |
| ⚙ **Config** | JSON configuration with per-provider API key storage |

---

## 📁 Project Structure

```
iraqiaware/
├── CMakeLists.txt            # CMake 3.16+ build system
├── src/
│   ├── main.cpp              # Entry point, logging init
│   ├── ScreenshotManager.cpp # Qt-based screen capture (5s interval)
│   ├── AIServiceLayer.cpp    # REST API calls to AI providers
│   ├── SecurityAnalyzer.cpp  # Parse AI output → structured threats
│   ├── ConfigManager.cpp     # JSON config read/write
│   ├── LanguageManager.cpp   # EN/AR translation lookup
│   ├── AlertSystem.cpp       # System-tray + in-app notifications
│   └── MainWindow.cpp        # Dark-theme Qt6 UI (tabs)
├── include/                  # Corresponding header files
├── resources/
│   ├── translations/
│   │   ├── en.json           # English UI strings
│   │   └── ar.json           # Arabic UI strings
│   └── styles/
│       └── dark_theme.qss    # Catppuccin Mocha Qt stylesheet
├── config/
│   └── default_config.json   # Bundled defaults (copied on first run)
├── LICENSE
└── .gitignore
```

---

## 🛠 Build Requirements

| Dependency | Version | Notes |
|---|---|---|
| C++ Compiler | C++17 | MSVC 2019+, GCC 9+, or Clang 10+ |
| CMake | ≥ 3.16 | |
| Qt | 6.x | Modules: Core, Widgets, Network, Gui |
| libcurl | any recent | For HTTP requests (or use Qt Network only) |
| nlohmann/json | ≥ 3.11 | Auto-fetched via FetchContent if not found |
| spdlog | ≥ 1.13 | Auto-fetched via FetchContent if not found |

### Windows (MSVC + vcpkg)

```powershell
# 1. Install vcpkg dependencies
vcpkg install curl qt6-base nlohmann-json spdlog

# 2. Configure
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# 3. Build
cmake --build build --config Release
```

### Windows (Easy one-command with CMake Presets)

```powershell
# Configure
cmake --preset windows-msvc-release

# Build
cmake --build --preset build-windows-msvc-release
```

### Windows (MinGW)

```bash
cmake -B build -G "MinGW Makefiles" \
      -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/mingw_64"
cmake --build build
```

---

## ⚙ Configuration

On first launch, `config/default_config.json` is copied to:

```
%LOCALAPPDATA%\MustafaCybersecurity\IraqiAware\config.json
```

Edit via the **Settings** tab in the application, or directly in the JSON file.

### Supported AI Providers

| Provider | Model field default | Notes |
|---|---|---|
| **OpenAI** | `gpt-4o` | Requires an OpenAI API key with vision access |
| **Google Gemini** | `gemini-1.5-pro` | Requires a Gemini API key |
| **Anthropic Claude** | `claude-3-5-sonnet-20241022` | Requires an Anthropic API key |
| **Ollama (local)** | `llava` | No key needed; run `ollama serve` locally |
| **Custom** | user-defined | Any OpenAI-compatible `/chat/completions` endpoint |

### External Alert Service API (Webhook)

In **Settings → Monitoring**:

- Enable **Forward alerts to external API**
- Set **Webhook URL**
- Optional: set **Webhook API Key** (sent as `Authorization: Bearer <key>`)

Payload example:

```json
{
  "source": "IraqiAware",
  "severity": "HIGH",
  "category": "Phishing/Suspicious Site",
  "message": "Suspicious login page detected",
  "recommendation": "Do not enter credentials and verify domain.",
  "timestamp": "2026-04-02T05:00:00Z"
}
```

### Real-time behavior

- Screenshot capture uses a precise timer.
- First screenshot is captured immediately when monitoring starts.
- Minimum configurable interval is **250 ms**.
- AI requests are coalesced (only the latest pending frame is queued) to avoid backlog and keep analysis near real-time.

---

## 🖼 Screenshots

> _Launch the application, open **Settings**, enter your AI API key, then press
> **▶ Start Monitoring** on the Dashboard to begin real-time analysis._

---

## 🔒 Security Notes

- API keys are stored **only** in the local user config file
  (`%LOCALAPPDATA%\…\config.json`) and are never committed to version control.
- Screenshots are sent **directly** to the configured AI endpoint over HTTPS;
  no third-party relay is involved.
- The application captures the **primary monitor** only.

---

## 📄 License

MIT – see [LICENSE](LICENSE).
