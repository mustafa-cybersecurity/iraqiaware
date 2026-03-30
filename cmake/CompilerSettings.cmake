# CompilerSettings.cmake
# ──────────────────────────────────────────────────────────────────────────────
# Configures compiler flags for MSVC (Windows x64) and GCC/Clang.
# ──────────────────────────────────────────────────────────────────────────────

if(MSVC)
    # ── MSVC (Visual Studio 2019/2022) ────────────────────────────────────────

    # Force x64 architecture (explicit, in case CMake generator doesn't enforce it)
    if(NOT CMAKE_GENERATOR_PLATFORM)
        set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "Platform" FORCE)
    endif()

    # C++ standard
    add_compile_options(/std:c++17)

    # Warning level
    add_compile_options(/W4)

    # Suppress a few noisy warnings from third-party headers
    add_compile_options(
        /wd4100   # unreferenced formal parameter (common in third-party code)
        /wd4127   # conditional expression is constant
        /wd4251   # DLL interface warning (Qt)
        /wd4702   # unreachable code
    )

    # Multi-processor compilation
    add_compile_options(/MP)

    # Unicode
    add_compile_definitions(UNICODE _UNICODE)

    # Windows version: require Windows 7 SP1 or higher
    add_compile_definitions(
        _WIN32_WINNT=0x0601
        WINVER=0x0601
    )

    # ── Release-specific ──────────────────────────────────────────────────────
    add_compile_options(
        $<$<CONFIG:Release>:/O2>      # maximum speed optimisation
        $<$<CONFIG:Release>:/Oi>      # generate intrinsic functions
        $<$<CONFIG:Release>:/Ot>      # favour fast code
        $<$<CONFIG:Release>:/GL>      # whole-program optimisation
        $<$<CONFIG:Release>:/GS->     # disable security checks (perf)
    )
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>    # link-time code generation
        $<$<CONFIG:Release>:/OPT:REF> # remove unused functions
        $<$<CONFIG:Release>:/OPT:ICF> # merge identical COMDATs
    )

    # ── Debug-specific ────────────────────────────────────────────────────────
    add_compile_options(
        $<$<CONFIG:Debug>:/Od>        # disable optimisations
        $<$<CONFIG:Debug>:/Zi>        # full debug information
        $<$<CONFIG:Debug>:/RTC1>      # runtime checks
    )

    # ── Runtime library ───────────────────────────────────────────────────────
    # Use dynamic CRT (/MD) to match Qt's default linkage.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

else()
    # ── GCC / Clang ──────────────────────────────────────────────────────────
    add_compile_options(-Wall -Wextra -Wpedantic)

    add_compile_options(
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Release>:-DNDEBUG>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
    )
endif()
