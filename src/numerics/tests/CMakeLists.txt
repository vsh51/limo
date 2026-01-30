include(GoogleTest)

add_executable(limo_numerics_fraction_tests
    fraction_tests.cpp
)
add_executable(limo_numerics_matrix_tests
    matrix_tests.cpp
)

target_link_libraries(limo_numerics_fraction_tests
    PRIVATE
        gtest_main
        limo_numerics
)
target_link_libraries(limo_numerics_matrix_tests
    PRIVATE
        gtest_main
        limo_numerics
)

gtest_discover_tests(limo_numerics_fraction_tests)
gtest_discover_tests(limo_numerics_matrix_tests)

if(TARGET tests)
    add_dependencies(tests limo_numerics_fraction_tests)
    add_dependencies(tests limo_numerics_matrix_tests)
endif()
