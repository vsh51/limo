add_library(limo_numerics
    include/limo/numerics/Matrix.hpp
    src/Fraction.cpp
)

target_include_directories(limo_numerics
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(limo_numerics
    PUBLIC
        limo_core
)

if(LIMO_BUILD_TESTS)
    add_subdirectory(tests)
endif()
