add_library(limo_core
    src/LinearProgram.cpp
    src/Solution.cpp
)

target_include_directories(limo_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
