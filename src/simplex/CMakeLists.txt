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
        limo_matrix
        limo_basis_artificial
        limo_basis_big_m
)
