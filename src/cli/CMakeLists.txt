add_executable(limo
    src/main.cpp
)

target_link_libraries(limo
    PRIVATE
        limo_io
        basis_finder
        limo_simplex
        limo_analysis
)
