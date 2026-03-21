add_executable(basis_finder_tests
    artificial_basis_finder_tests.cpp
    big_m_basis_finder_tests.cpp
)

target_link_libraries(basis_finder_tests
    PRIVATE
        basis_finder
        limo_core
        limo_numerics
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(basis_finder_tests)

if(TARGET tests)
    add_dependencies(tests basis_finder_tests)
endif()
