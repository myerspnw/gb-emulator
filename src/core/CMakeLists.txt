# The emulation core: pure logic, no I/O, no SDL.
# This library is what the integration tests link against headlessly.

add_library(gbe_core STATIC
    gameboy.cpp
)

target_include_directories(gbe_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Core depends on fmt for formatting only. It deliberately does NOT
# link spdlog or any other I/O-performing library — the architecture
# invariant is that the core does no I/O. Diagnostic output and error
# logging happen in the frontend, which formats and emits std::expected
# errors bubbled up from the core.
target_link_libraries(gbe_core
    PUBLIC
        fmt::fmt
    PRIVATE
        gbe_warnings
        gbe_sanitizers
)
