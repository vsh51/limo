add_library(limo_basis_big_m
    src/BigMBasisFinder.cpp
)

target_include_directories(limo_basis_big_m
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(limo_basis_big_m
    PUBLIC
        limo_core
        limo_matrix
)
