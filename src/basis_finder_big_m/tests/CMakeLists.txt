add_executable(limo_basis_big_m_tests
    big_m_basis_finder_tests.cpp
)

target_link_libraries(limo_basis_big_m_tests
    PRIVATE
        gtest_main
        limo_basis_big_m
        limo_core
        limo_numerics
)

include(GoogleTest)
gtest_discover_tests(limo_basis_big_m_tests)

if(TARGET tests)
    add_dependencies(tests limo_basis_big_m_tests)
endif()
