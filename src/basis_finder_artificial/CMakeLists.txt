add_library(limo_basis_artificial
    src/ArtificialBasisFinder.cpp
)

target_include_directories(limo_basis_artificial
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(limo_basis_artificial
    PUBLIC
        limo_core
        limo_matrix
)
