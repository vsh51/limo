

cmake_minimum_required(VERSION 3.20)
project(LIMO_SRC)

# Ensure CTest is enabled for all subdirectories
enable_testing()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(core)
add_subdirectory(thread_pool)
add_subdirectory(numerics)
add_subdirectory(basis_finder)
add_subdirectory(basis_finder/tests)
add_subdirectory(simplex)
add_subdirectory(analysis)

if(LIMO_BUILD_CLI)
    add_subdirectory(cli)
endif()
