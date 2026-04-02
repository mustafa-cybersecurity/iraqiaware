cmake_minimum_required(VERSION 3.16)

project(iraqiaware
    VERSION 1.0.0
    DESCRIPTION "CyberSecurity Awareness Monitorizer Powered by AI"
    LANGUAGES CXX
)

# ── C++ standard ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── CMake modules ─────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerSettings)

# ── vcpkg integration ─────────────────────────────────────────────────────────
# If VCPKG_ROOT is set in environment, use it automatically.
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain" FORCE)
endif()

# Force x64 on Windows
if(WIN32)
    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg triplet")
    endif()
endif()

# ── Output directories ────────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/x64/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/x64/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/x64/$<CONFIG>")

# ── Qt6 ───────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

# ── Third-party libraries (via vcpkg) ────────────────────────────────────────
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

# ── Source files ─────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ScreenshotManager.cpp
    src/AIServiceLayer.cpp
    src/SecurityAnalyzer.cpp
    src/ConfigManager.cpp
    src/LanguageManager.cpp
    src/AlertSystem.cpp
    src/LoggingSystem.cpp
    src/AsyncAnalyzer.cpp
    src/CacheManager.cpp
)

set(HEADERS
    include/MainWindow.h
    include/ScreenshotManager.h
    include/AIServiceLayer.h
    include/SecurityAnalyzer.h
    include/ConfigManager.h
    include/LanguageManager.h
    include/AlertSystem.h
    include/LoggingSystem.h
    include/ThreatModel.h
    include/AsyncAnalyzer.h
    include/CacheManager.h
)

# ── Executable ────────────────────────────────────────────────────────────────
if(WIN32)
    # WIN32 flag: no console window in Release builds
    qt_add_executable(iraqiaware WIN32 ${SOURCES} ${HEADERS})
else()
    qt_add_executable(iraqiaware ${SOURCES} ${HEADERS})
endif()

# ── Include directories ───────────────────────────────────────────────────────
target_include_directories(iraqiaware PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(iraqiaware PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    CURL::libcurl
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(WIN32)
    target_link_libraries(iraqiaware PRIVATE
        gdiplus
        shell32
        user32
        winmm
    )
endif()

# ── Copy resources to build output ───────────────────────────────────────────
add_custom_command(TARGET iraqiaware POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:iraqiaware>/resources"
    COMMENT "Copying resources..."
)

add_custom_command(TARGET iraqiaware POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/config"
        "$<TARGET_FILE_DIR:iraqiaware>/config"
    COMMENT "Copying config files..."
)

# ── Windows: deploy Qt DLLs ──────────────────────────────────────────────────
if(WIN32)
    # windeployqt bundles the required Qt DLLs next to the .exe
    add_custom_command(TARGET iraqiaware POST_BUILD
        COMMAND "${Qt6_DIR}/../../../bin/windeployqt.exe"
            --no-compiler-runtime
            --no-translations
            "$<TARGET_FILE:iraqiaware>"
        COMMENT "Deploying Qt runtime..."
        VERBATIM
    )
endif()

# ── Install rules ─────────────────────────────────────────────────────────────
install(TARGETS iraqiaware
    RUNTIME DESTINATION bin
)
install(DIRECTORY resources/ DESTINATION bin/resources)
install(DIRECTORY config/    DESTINATION bin/config)

# ── CPack / NSIS installer (optional) ────────────────────────────────────────
set(CPACK_PACKAGE_NAME        "IraqiAware")
set(CPACK_PACKAGE_VERSION     "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR      "IraqiAware")
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_GENERATOR           "NSIS;ZIP")
set(CPACK_NSIS_DISPLAY_NAME   "IraqiAware CyberSecurity Monitor")
set(CPACK_NSIS_INSTALL_ROOT   "$PROGRAMFILES64")
include(CPack)

# ── Print summary ─────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "=== IraqiAware Build Configuration ===")
message(STATUS "  Version     : ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output Dir  : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "=======================================")
message(STATUS "")
