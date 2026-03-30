cmake_minimum_required(VERSION 3.16)

project(IraqiAware
    VERSION 1.0.0
    DESCRIPTION "CyberSecurity Awareness Monitorizer Powered by AI"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Find Qt ──────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Gui
)

# ── Find or fetch nlohmann/json ──────────────────────────────────────────────
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ── Find or fetch spdlog ─────────────────────────────────────────────────────
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        URL https://github.com/gabime/spdlog/archive/refs/tags/v1.13.0.tar.gz
        URL_HASH SHA256=534f2ee1a4dcbeb22249856edfb2be3b4d34d5e29bffd7e64510c0f43a7e8b38
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# ── Find CURL ────────────────────────────────────────────────────────────────
find_package(CURL REQUIRED)

# ── Source files ─────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/ScreenshotManager.cpp
    src/AIServiceLayer.cpp
    src/SecurityAnalyzer.cpp
    src/ConfigManager.cpp
    src/LanguageManager.cpp
    src/AlertSystem.cpp
    src/MainWindow.cpp
)

set(HEADERS
    include/ScreenshotManager.h
    include/AIServiceLayer.h
    include/SecurityAnalyzer.h
    include/ConfigManager.h
    include/LanguageManager.h
    include/AlertSystem.h
    include/MainWindow.h
)

# ── Executable ───────────────────────────────────────────────────────────────
qt_add_executable(IraqiAware
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(IraqiAware PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(IraqiAware PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    ${CURL_LIBRARIES}
)

# ── Windows-specific settings ────────────────────────────────────────────────
if(WIN32)
    target_link_libraries(IraqiAware PRIVATE
        gdi32
        gdiplus
        user32
        dwmapi
    )
    set_target_properties(IraqiAware PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ── Copy resources to build directory ────────────────────────────────────────
add_custom_command(TARGET IraqiAware POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:IraqiAware>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/config
        $<TARGET_FILE_DIR:IraqiAware>/config
)

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS IraqiAware
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
