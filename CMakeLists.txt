cmake_minimum_required(VERSION 3.20)

project(LIMO VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(LIMO_BUILD_CLI "Build CLI application" ON)
option(LIMO_BUILD_DOCS "Build Doxygen documentation" OFF)
option(LIMO_BUILD_TESTS "Build unit tests" ON)

if(LIMO_BUILD_TESTS)
	include(FetchContent)
	FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)
	enable_testing()

	add_custom_target(tests
		COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
		COMMENT "Building and running unit tests"
	)
endif()

if(LIMO_BUILD_DOCS)
	find_package(Doxygen QUIET)
	if(DOXYGEN_FOUND)
		set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
		set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
		configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

		add_custom_target(docs
			COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating documentation with Doxygen."
			VERBATIM
		)
	else()
		message(WARNING "Doxygen not found.")
	endif()
endif()

add_subdirectory(libs)
add_subdirectory(src)
