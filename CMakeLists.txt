# Minimum CMake version
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project name
set(LIB_NAME "CCSDSPack")

#MAJOR: Increases when there are breaking changes that are incompatible with previous versions. A major version change indicates significant updates or overhauls, and often requires developers to modify their code to stay compatible.

#MINOR: Increases when new features or functionality are added in a backward-compatible way. These updates enhance the software but do not break existing functionality.

#PATCH: Increases for backward-compatible bug fixes or small improvements. A patch update fixes issues, improves stability, or makes minor improvements without adding new features.

# Versioning
set(MAJOR "0")
set(MINOR "3")
set(PATCH "2")

set(VER "-v${MAJOR}.${MINOR}.${PATCH}")
set(LIB_VERSIONED "${LIB_NAME}${VER}")
set(TESTER_EXEC "${LIB_NAME}_tester")

project(${LIB_VERSIONED})

# Define directories
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/inc")
set(TEST_SOURCE_DIR "${CMAKE_SOURCE_DIR}/test/src")
set(TEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/test/inc")

# Set output directories for different OSes
if (WIN32)
    set(LIBRARY_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib")
    set(BINARY_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
else()
    set(LIBRARY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/lib")
    set(BINARY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")
endif()

# Add include directories
include_directories(${INCLUDE_DIR})

# Collect all source files in the src folder for the library
file(GLOB LIBRARY_SOURCES "${SOURCE_DIR}/*.cpp")

# Create the shared library target
add_library(${LIB_VERSIONED} SHARED ${LIBRARY_SOURCES})

# Ensure Windows exports all symbols when building a shared library
if (WIN32)
    set_target_properties(${LIB_VERSIONED} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Set the output directories for different platforms
set_target_properties(${LIB_VERSIONED} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}
)

# Collect all source files in the test src folder for the test executable
file(GLOB TEST_SOURCES "${TEST_SOURCE_DIR}/*.cpp")


# Create the test executable target
add_executable(${TESTER_EXEC} ${TEST_SOURCES})

# Add include directories to the test target
target_include_directories(${TESTER_EXEC} PRIVATE ${INCLUDE_DIR} ${TEST_INCLUDE_DIR})

# Link the test executable with the library
target_link_libraries(${TESTER_EXEC} PRIVATE ${LIB_VERSIONED})

# Set the output directory for the test executable
set_target_properties(${TESTER_EXEC} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}
)
