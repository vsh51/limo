add_library(limo_analysis
    src/Duality.cpp
    src/Sensitivity.cpp
)

target_include_directories(limo_analysis
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(limo_analysis
    PUBLIC
        limo_core
        limo_simplex
)
