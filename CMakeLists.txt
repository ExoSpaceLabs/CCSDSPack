# Minimum CMake version
cmake_minimum_required(VERSION 3.16)


# Project name
set(LIB_NAME "CCSDSPack")

#MAJOR: Increases when there are breaking changes that are incompatible with previous versions. A major version change indicates significant updates or overhauls, and often requires developers to modify their code to stay compatible.

#MINOR: Increases when new features or functionality are added in a backward-compatible way. These updates enhance the software but do not break existing functionality.

#PATCH: Increases for backward-compatible bug fixes or small improvements. A patch update fixes issues, improves stability, or makes minor improvements without adding new features.

# Versioning
set(MAJOR "1")
set(MINOR "0")
set(PATCH "0")

set(VER "-v${MAJOR}.${MINOR}.${PATCH}")
set(LIB_VERSIONED "${LIB_NAME}${VER}")

project(${LIB_NAME} VERSION ${MAJOR}.${MINOR}.${PATCH} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Flags:")
option(ENABLE_TESTER "Build the CCSDSPack tester executable" ON)
message(STATUS "  -DENABLE_TESTER=${ENABLE_TESTER}")
option(ENABLE_ENCODER "Build the CCSDSPack encoder executable" ON)
message(STATUS "  -DENABLE_ENCODER=${ENABLE_ENCODER}")
option(ENABLE_DECODER "Build the CCSDSPack decoder executable" ON)
message(STATUS "  -DENABLE_DECODER=${ENABLE_DECODER}")
option(ENABLE_VALIDATOR "Build the CCSDSPack validator executable" ON)
message(STATUS "  -DENABLE_VALIDATOR=${ENABLE_VALIDATOR}")

# Define directories
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/inc")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# for standard install directories
include(GNUInstallDirs)

# Set output directories for different OSes
if (WIN32)
    set(LIBRARY_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib")
    set(BINARY_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
else()
    set(LIBRARY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/lib")
    set(BINARY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")
endif()

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
add_library(ccsdspack::core ALIAS ${LIB_NAME})

# Add include directories
include_directories(${INCLUDE_DIR})

set_target_properties(${LIB_NAME} PROPERTIES
        VERSION ${MAJOR}.${MINOR}.${PATCH}
        SOVERSION ${MAJOR}
        OUTPUT_NAME ccsdspack
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

install(TARGETS ${LIB_NAME}
        EXPORT CCSDSPackTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll / executables on Windows
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .so/.dylib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .lib (import/static)
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${INCLUDE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
)

# -----------------------------
# Exported targets + Config package (enable find_package(CCSDSPack))
# -----------------------------
include(CMakePackageConfigHelpers)

# Write a minimal Config.cmake template on the fly (no repo file needed)
set(_ccsds_cfg_in "${CMAKE_CURRENT_BINARY_DIR}/CCSDSPackConfig.cmake.in")
file(WRITE "${_ccsds_cfg_in}"
        "@PACKAGE_INIT@\n\
include(CMakeFindDependencyMacro)\n\
include(\"\${CMAKE_CURRENT_LIST_DIR}/CCSDSPackTargets.cmake\")\n")

# Generate Config and ConfigVersion files
configure_package_config_file(
        "${_ccsds_cfg_in}"
        "${CMAKE_CURRENT_BINARY_DIR}/CCSDSPackConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CCSDSPack"
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/CCSDSPackConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

# Install the exported targets and config files
install(EXPORT CCSDSPackTargets
        NAMESPACE ccsdspack::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CCSDSPack
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CCSDSPackConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/CCSDSPackConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CCSDSPack
)


include(${CMAKE_SOURCE_DIR}/cmake/cpack.cmake)

# -----------------------------
# Notes for CI (Windows/Linux/macOS)
# -----------------------------
# - This CMakeLists builds a shared library by default and installs headers + CMake package files.
# - Linux/macOS: you can run `cpack` after `cmake --install` to build .deb/.packages/.tgz if you enable CPack in a separate step.
# - Windows (2022 runner): you'll get ccsdspack.dll + ccsdspack.lib; install() places the DLL into bin/ and the import lib into lib/.