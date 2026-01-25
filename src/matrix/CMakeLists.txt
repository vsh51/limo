add_library(limo_matrix
    src/Matrix.cpp
    src/Fraction.cpp
)

target_include_directories(limo_matrix
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(limo_matrix
    PUBLIC
        limo_core
)

if(LIMO_BUILD_TESTS)
    add_subdirectory(tests)
endif()
