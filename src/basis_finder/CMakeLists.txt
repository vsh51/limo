# CMakeLists.txt for basis_finder module
cmake_minimum_required(VERSION 3.10)
project(basis_finder)

add_library(basis_finder
    src/ArtificialBasisFinder.cpp
    src/BigMBasisFinder.cpp
)

target_include_directories(basis_finder PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/numerics/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(basis_finder PUBLIC limo_core)

if(LIMO_BUILD_TESTS)
    add_subdirectory(tests)
endif()
