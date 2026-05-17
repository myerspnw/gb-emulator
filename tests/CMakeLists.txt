include(GoogleTest)

add_executable(gbe_unit_tests
    unit/sanity_test.cpp
)

target_link_libraries(gbe_unit_tests
    PRIVATE
        gbe_core
        GTest::gtest_main
        gbe_warnings
        gbe_sanitizers
)

gtest_discover_tests(gbe_unit_tests)

# Architecture invariant: the core does not include the frontend, SDL3,
# or any logging library. Enforced as a ctest test so a violation fails
# CI and `ctest` locally. See cmake/CheckCoreBoundary.cmake.
add_test(
    NAME core_boundary
    COMMAND ${CMAKE_COMMAND}
            -DCORE_DIR=${PROJECT_SOURCE_DIR}/src/core
            -P ${PROJECT_SOURCE_DIR}/cmake/CheckCoreBoundary.cmake
)
