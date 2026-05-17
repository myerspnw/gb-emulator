add_subdirectory(core)
add_subdirectory(frontend)

add_executable(gbe main.cpp)
# gbe_frontend PUBLIC-links gbe_core, so listing gbe_core here too would
# pull it in twice and trip Apple ld's "duplicate libraries" warning.
target_link_libraries(gbe
    PRIVATE
        gbe_frontend
        gbe_warnings
        gbe_sanitizers
)
