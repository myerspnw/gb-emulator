add_subdirectory(core)
add_subdirectory(frontend)

add_executable(gbe main.cpp)
target_link_libraries(gbe
    PRIVATE
        gbe_core
        gbe_frontend
        gbe_warnings
        gbe_sanitizers
)
