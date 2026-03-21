add_library(limo_simplex
    src/SimplexSolver.cpp
    src/ModifiedSimplexSolver.cpp
)

target_include_directories(limo_simplex
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(limo_simplex
    PUBLIC
        limo_core
        limo_numerics
        basis_finder
        limo_io
)

if(LIMO_BUILD_TESTS)
    add_subdirectory(tests)
endif()
