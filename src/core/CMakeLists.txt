# The emulation core: pure logic, no I/O, no SDL.
# This library is what the integration tests link against headlessly.

add_library(gbe_core STATIC
    gameboy.cpp
)

target_include_directories(gbe_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(gbe_core
    PUBLIC
        fmt::fmt
        spdlog::spdlog
    PRIVATE
        gbe_warnings
        gbe_sanitizers
)
