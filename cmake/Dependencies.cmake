include(FetchContent)

# Pin versions for reproducibility. Bump deliberately.
set(GBE_SDL_TAG       "release-3.4.8")
set(GBE_FMT_TAG       "12.1.0")
set(GBE_SPDLOG_TAG    "v1.17.0")
set(GBE_GTEST_TAG     "v1.17.0")

# ─── fmt ───────────────────────────────────────────────────────────────
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        ${GBE_FMT_TAG}
    GIT_SHALLOW    TRUE
    SYSTEM
)

# ─── spdlog ────────────────────────────────────────────────────────────
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        ${GBE_SPDLOG_TAG}
    GIT_SHALLOW    TRUE
    SYSTEM
)

# ─── SDL3 ──────────────────────────────────────────────────────────────
set(SDL_TEST     OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_SHARED   OFF CACHE BOOL "" FORCE)
set(SDL_STATIC   ON  CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        ${GBE_SDL_TAG}
    GIT_SHALLOW    TRUE
    SYSTEM
)

# ─── GoogleTest ────────────────────────────────────────────────────────
if(GBE_BUILD_TESTS)
    set(BUILD_GMOCK    OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST  OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        ${GBE_GTEST_TAG}
        GIT_SHALLOW    TRUE
        SYSTEM
    )
endif()

# Make available — order matters: fmt before spdlog
FetchContent_MakeAvailable(fmt spdlog SDL3)
if(GBE_BUILD_TESTS)
    FetchContent_MakeAvailable(googletest)
endif()
