add_executable(limo_thread_pool_tests
    thread_pool_tests.cpp
)

target_link_libraries(limo_thread_pool_tests
    PRIVATE
        gtest_main
        limo_thread_pool
)

include(GoogleTest)

gtest_discover_tests(limo_thread_pool_tests)

if(TARGET tests)
    add_dependencies(tests limo_thread_pool_tests)
endif()
