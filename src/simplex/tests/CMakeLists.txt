add_executable(simplex_solver_tests
    simplex_solver_tests.cpp
)

target_link_libraries(simplex_solver_tests
    PRIVATE
        limo_simplex
        limo_io
        GTest::gtest_main
)

gtest_discover_tests(simplex_solver_tests)

if(TARGET tests)
    add_dependencies(tests simplex_solver_tests)
endif()
