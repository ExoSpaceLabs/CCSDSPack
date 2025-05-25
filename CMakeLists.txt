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
set(MINOR "8")
set(PATCH "0")

message(STATUS "Flags:")
option(ENABLE_TESTER "Build the CCSDSPack tester executable" ON)
message(STATUS "  -DENABLE_TESTER=${ENABLE_TESTER}")
option(ENABLE_ENCODER "Build the CCSDSPack encoder executable" ON)
message(STATUS "  -DENABLE_ENCODER=${ENABLE_ENCODER}")
option(ENABLE_DECODER "Build the CCSDSPack decoder executable" ON)
message(STATUS "  -DENABLE_DECODER=${ENABLE_DECODER}")
option(ENABLE_VALIDATOR "Build the CCSDSPack validator executable" ON)
message(STATUS "  -DENABLE_VALIDATOR=${ENABLE_VALIDATOR}")


set(VER "-v${MAJOR}.${MINOR}.${PATCH}")
set(LIB_VERSIONED "${LIB_NAME}${VER}")

project(${LIB_VERSIONED})

# Define directories
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/inc")

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
set(LIBRARY_SOURCES
        "${SOURCE_DIR}/CCSDSDataField.cpp"
        "${SOURCE_DIR}/CCSDSHeader.cpp"
        "${SOURCE_DIR}/CCSDSManager.cpp"
        "${SOURCE_DIR}/CCSDSPacket.cpp"
        "${SOURCE_DIR}/CCSDSUtils.cpp"
        "${SOURCE_DIR}/CCSDSConfig.cpp"
        "${SOURCE_DIR}/CCSDSValidator.cpp"
        "${SOURCE_DIR}/PusServices.cpp"
)
# Create the shared library target
add_library(${LIB_NAME} SHARED ${LIBRARY_SOURCES})
set_target_properties(${LIB_NAME} PROPERTIES
        VERSION ${MAJOR}.${MINOR}.${PATCH}
        SOVERSION ${MAJOR}
)

# Ensure Windows exports all symbols when building a shared library
if (WIN32)
    set_target_properties(${LIB_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Set the output directories for different platforms
set_target_properties(${LIB_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}
)

# Enables build of tester
if(ENABLE_TESTER)
    include(${CMAKE_SOURCE_DIR}/cmake/tester.cmake)
endif ()

# Enables build of encoder
if(ENABLE_ENCODER)
    include(${CMAKE_SOURCE_DIR}/cmake/encoder.cmake)
endif ()

# Enables build of decoder
if(ENABLE_DECODER)
    include(${CMAKE_SOURCE_DIR}/cmake/decoder.cmake)
endif ()

# Enables build of validator
if(ENABLE_VALIDATOR)
    include(${CMAKE_SOURCE_DIR}/cmake/validator.cmake)
endif ()


if(WIN32)
    # Install DLL to the bin directory
    install(TARGETS ${LIB_NAME}
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib)

    # Install headers
    install(DIRECTORY ${INCLUDE_DIR}/
            DESTINATION include
            FILES_MATCHING PATTERN "*.h")
else()
    # Default Linux installation
    install(TARGETS ${LIB_NAME}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            RUNTIME DESTINATION bin)

    # Install headers
    install(DIRECTORY ${INCLUDE_DIR}/
            DESTINATION include
            FILES_MATCHING PATTERN "*.h")
endif()

