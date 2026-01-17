add_executable(limo
    src/main.cpp
)

target_link_libraries(limo
    PRIVATE
        limo_core
        limo_matrix
        limo_basis_artificial
        limo_basis_big_m
        limo_simplex
        limo_analysis
)
