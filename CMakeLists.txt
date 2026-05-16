cmake_minimum_required(VERSION 3.25)

project(gb_emulator
    VERSION 0.1.0
    DESCRIPTION "A modern C++23 Game Boy emulator"
    LANGUAGES CXX
)

# ─── Project-wide settings ─────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd / clang-tidy

# Default to Release if no build type specified (single-config generators)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# Use lld if available (much faster linking)
find_program(LLD_PROGRAM lld)
if(LLD_PROGRAM AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_link_options(-fuse-ld=lld)
endif()

# ─── Options ───────────────────────────────────────────────────────────
option(GBE_BUILD_TESTS    "Build unit and integration tests" ON)
option(GBE_ENABLE_ASAN    "Enable AddressSanitizer"          OFF)
option(GBE_ENABLE_UBSAN   "Enable UndefinedBehaviorSanitizer" OFF)
option(GBE_ENABLE_TSAN    "Enable ThreadSanitizer"           OFF)
option(GBE_ENABLE_WERROR  "Treat warnings as errors"         ON)

# ─── Compiler warnings & sanitizers ────────────────────────────────────
include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)

# ─── Dependencies (FetchContent) ───────────────────────────────────────
include(cmake/Dependencies.cmake)

# ─── Subdirectories ────────────────────────────────────────────────────
add_subdirectory(src)

if(GBE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
