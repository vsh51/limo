add_library(limo_core
    src/LinearProgram.cpp
    src/Solution.cpp
)

target_include_directories(limo_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(limo_core
    PUBLIC
        limo_numerics
)

if(LIMO_BUILD_TESTS)
    add_subdirectory(tests)
endif()