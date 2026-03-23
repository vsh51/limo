add_executable(io_tests
    json_serializer_tests.cpp
    input_parser_tests.cpp
    output_builder_tests.cpp
    file_parser_tests.cpp
)

target_link_libraries(io_tests
    PRIVATE
        limo_io
        gtest
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(io_tests)

if(TARGET tests)
    add_dependencies(tests io_tests)
endif()
