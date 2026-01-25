add_executable(limo_matrix_fraction_tests
    fraction_tests.cpp
)

target_link_libraries(limo_matrix_fraction_tests
    PRIVATE
        gtest_main
        limo_matrix
)

include(GoogleTest)

gtest_discover_tests(limo_matrix_fraction_tests)

if(TARGET tests)
    add_dependencies(tests limo_matrix_fraction_tests)
endif()
