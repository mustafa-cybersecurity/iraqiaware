# FindQt6.cmake
# ──────────────────────────────────────────────────────────────────────────────
# Helper module that locates Qt6.  CMake's built-in Qt6Config.cmake is preferred;
# this module only provides additional search hints for common installation paths
# on Windows and sets CMAKE_PREFIX_PATH so find_package(Qt6 …) succeeds.
# ──────────────────────────────────────────────────────────────────────────────

# If Qt6_DIR is already set by the user or vcpkg, respect it.
if(DEFINED Qt6_DIR)
    return()
endif()

# ── Common Windows installation paths ────────────────────────────────────────
if(WIN32)
    set(_qt6_hints
        "C:/Qt/6.7.0/msvc2019_64"
        "C:/Qt/6.7.0/msvc2022_64"
        "C:/Qt/6.6.3/msvc2019_64"
        "C:/Qt/6.6.3/msvc2022_64"
        "C:/Qt/6.5.3/msvc2019_64"
        "C:/Qt/6.5.3/msvc2022_64"
        "C:/Qt/6.4.3/msvc2019_64"
        "$ENV{QT_DIR}"
        "$ENV{Qt6_DIR}"
    )

    foreach(_hint IN LISTS _qt6_hints)
        if(EXISTS "${_hint}/lib/cmake/Qt6")
            list(APPEND CMAKE_PREFIX_PATH "${_hint}")
            message(STATUS "Qt6 hint added to CMAKE_PREFIX_PATH: ${_hint}")
            break()
        endif()
    endforeach()
endif()

# ── macOS / Linux common paths ────────────────────────────────────────────────
if(UNIX AND NOT APPLE)
    list(APPEND CMAKE_PREFIX_PATH
        "/opt/Qt/6.7.0/gcc_64"
        "/usr/lib/x86_64-linux-gnu/cmake/Qt6"
    )
endif()

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH
        "/usr/local/opt/qt@6"
        "$ENV{HOME}/Qt/6.7.0/macos"
    )
endif()
