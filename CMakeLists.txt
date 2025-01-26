# Minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Project name
set(LIB_NAME "CCSDSPack")


#MAJOR: Increases when there are breaking changes that are incompatible with previous versions. A major version change indicates significant updates or overhauls, and often requires developers to modify their code to stay compatible.

#MINOR: Increases when new features or functionality are added in a backward-compatible way. These updates enhance the software but do not break existing functionality.

#PATCH: Increases for backward-compatible bug fixes or small improvements. A patch update fixes issues, improves stability, or makes minor improvements without adding new features.

set(MAJOR "0")
set(MINOR "2")
set(PATCH "0")

set(VER "-v${MAJOR}.${MINOR}.${PATCH}")
set(LIB_VERSIONED "${LIB_NAME}${VER}")
set(TESTER_EXEC "${LIB_NAME}_tester")

project(${LIB_VERSIONED})

# Define directories
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/inc")
set(LIBRARY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/lib")
set(BINARY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")

# Add include directories
include_directories(${INCLUDE_DIR})
# Collect all source files in the src folder for the library
file(GLOB LIBRARY_INCLUDES "${INCLUDE_DIR}/*.h")

# Collect all source files in the src folder for the library
file(GLOB LIBRARY_SOURCES "${SOURCE_DIR}/*.cpp")

# Create the library target
add_library(${LIB_VERSIONED} SHARED ${LIBRARY_SOURCES})

# Set the output directory for the library
set_target_properties(${LIB_VERSIONED} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
)

# Collect source files for the test executable (assuming it's in src/test.cpp)
set(TEST_SOURCES "${TEST_DIR}/CCSDSPack_test.cpp")

# Create the test executable target
add_executable(${TESTER_EXEC} ${TEST_SOURCES})

# Add include directories to the test target
target_include_directories(${TESTER_EXEC} PRIVATE ${INCLUDE_DIR})

# Link the test executable with the library
target_link_libraries(${TESTER_EXEC} PRIVATE ${LIB_VERSIONED})

# Set the output directory for the test executable
set_target_properties(${TESTER_EXEC} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}
)

