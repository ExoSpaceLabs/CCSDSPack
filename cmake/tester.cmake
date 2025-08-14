

set(TESTER_EXEC "${LIB_NAME}_tester")

message(STATUS "Building: ${TESTER_EXEC}")

set(TEST_SOURCE_DIR "${CMAKE_SOURCE_DIR}/test/src")
set(TEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/test/inc")
set(TEST_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/test/test_resources")


# Collect all source files in the test src folder for the test executable
file(GLOB TEST_SOURCES "${TEST_SOURCE_DIR}/*.cpp")


# Collect all test test_resources from the test/test_resources directory
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
            BUILD_RPATH "/usr/local/lib:${LIBRARY_OUTPUT_DIR}:${CMAKE_BINARY_DIR}/lib"  # During build time, look in these paths for libraries
            INSTALL_RPATH "/usr/local/lib:${LIBRARY_OUTPUT_DIR}:${CMAKE_BINARY_DIR}/lib" # After installation, look in these paths for libraries
    )
elseif(WIN32)
    # Windows doesn't use RPATH. DLLs are usually placed in the same directory as the EXE or specified in PATH.
    set_target_properties(${TESTER_EXEC} PROPERTIES
            # No need to set RPATH on Windows
            RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}  # Ensure DLL is next to the executable
    )
endif()

install(TARGETS ${TESTER_EXEC}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll / executables on Windows
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install test_resources to the bin directory
install(DIRECTORY ${TEST_RESOURCE_DIR}
        DESTINATION "${CMAKE_INSTALL_BINDIR}")
