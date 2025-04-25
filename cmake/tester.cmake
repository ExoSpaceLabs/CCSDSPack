

set(TESTER_EXEC "${LIB_NAME}_tester")

message(STATUS "Building: ${TESTER_EXEC}")

set(TEST_SOURCE_DIR "${CMAKE_SOURCE_DIR}/test/src")
set(TEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/test/inc")
set(TEST_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/test/resources")


# Collect all source files in the test src folder for the test executable
file(GLOB TEST_SOURCES "${TEST_SOURCE_DIR}/*.cpp")


# Collect all test resources from the test/resources directory
file(GLOB TEST_RESOURCES "${TEST_RESOURCE_DIR}/*")

# Create the test executable target
add_executable(${TESTER_EXEC} ${TEST_SOURCES})

# Add include directories to the test target
target_include_directories(${TESTER_EXEC} PRIVATE ${INCLUDE_DIR} ${TEST_INCLUDE_DIR})

# Link the test executable with the library
target_link_libraries(${TESTER_EXEC} PRIVATE ${LIB_NAME})

# Set the output directory for the test executable
set_target_properties(${TESTER_EXEC} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}          # Specifies where the executable is placed
)

# Platform-specific RPATH settings
if(UNIX)
    # Linux or macOS
    set_target_properties(${TESTER_EXEC} PROPERTIES
            BUILD_RPATH "/usr/local/lib:${LIBRARY_OUTPUT_DIR}"  # During build time, look in these paths for libraries
            INSTALL_RPATH "/usr/local/lib:${LIBRARY_OUTPUT_DIR}" # After installation, look in these paths for libraries
    )
elseif(WIN32)
    # Windows doesn't use RPATH. DLLs are usually placed in the same directory as the EXE or specified in PATH.
    set_target_properties(${TESTER_EXEC} PROPERTIES
            # No need to set RPATH on Windows
            RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}  # Ensure DLL is next to the executable
    )
endif()

# Install resources to the bin directory
file(MAKE_DIRECTORY "${BINARY_OUTPUT_DIR}/test_resources")
file(COPY ${TEST_RESOURCES}
        DESTINATION "${BINARY_OUTPUT_DIR}/test_resources")
