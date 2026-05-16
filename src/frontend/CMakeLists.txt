# The frontend: SDL3 window, audio, input. Talks to gbe_core.

add_library(gbe_frontend STATIC
    app.cpp
)

target_include_directories(gbe_frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(gbe_frontend
    PUBLIC
        gbe_core
        SDL3::SDL3-static
    PRIVATE
        gbe_warnings
        gbe_sanitizers
)
