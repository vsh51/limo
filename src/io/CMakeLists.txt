add_library(limo_io
    src/JsonSerializer.cpp
    src/InputParser.cpp
    src/FileParser.cpp
    src/OutputBuilder.cpp
    src/IterationCollector.cpp
)

target_include_directories(limo_io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(limo_io
    PUBLIC
        limo_core
        limo_numerics
        nlohmann_json::nlohmann_json
)

if(LIMO_BUILD_TESTS)
    add_subdirectory(tests)
endif()
