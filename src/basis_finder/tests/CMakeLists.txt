
# CMakeLists.txt for basis_finder/tests

cmake_minimum_required(VERSION 3.10)
project(basis_finder_tests)

# GoogleTest FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(basis_finder_tests
    artificial_basis_finder_tests.cpp
    big_m_basis_finder_tests.cpp
)

target_link_libraries(basis_finder_tests PRIVATE basis_finder limo_core limo_numerics gtest gtest_main)

target_include_directories(basis_finder_tests PRIVATE
    ../include
    ../../core/include
    ../../numerics/include
)

# Enable GoogleTest CTest integration
include(GoogleTest)

# Register tests with CTest
gtest_discover_tests(basis_finder_tests)
