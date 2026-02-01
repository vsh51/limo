add_executable(limo_core_linear_program_tests
    linear_program_tests.cpp
)

target_link_libraries(limo_core_linear_program_tests
    PRIVATE
        gtest_main
        limo_core
)

include(GoogleTest)

gtest_discover_tests(limo_core_linear_program_tests)

if(TARGET tests)
    add_dependencies(tests limo_core_linear_program_tests)
endif()
