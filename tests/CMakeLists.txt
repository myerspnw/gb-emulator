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
