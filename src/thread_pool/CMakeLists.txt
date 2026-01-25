add_library(limo_thread_pool
    src/ThreadPool.cpp
)

target_include_directories(limo_thread_pool
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(LIMO_BUILD_TESTS)
    add_subdirectory(tests)
endif()
