# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

set(LIB_NAME "CCSDSPack")
set(MAJOR "2")
set(MINOR "0")
set(PATCH "0")
set(VER "-v${MAJOR}.${MINOR}.${PATCH}")
set(LIB_VERSIONED "${LIB_NAME}${VER}")

project(${LIB_NAME} VERSION ${MAJOR}.${MINOR}.${PATCH} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CCSDSPACK_BUILD_MCU "Build the CCSDSPack static library for MCU/bare-metal consumers." OFF)
set(CCSDSPACK_MCU_FLAGS "" CACHE STRING "Extra MCU-specific compiler flags (e.g. -fno-exceptions -fno-rtti)")

message(STATUS "Flags:")
message(STATUS "  -DCCSDSPACK_BUILD_MCU=${CCSDSPACK_BUILD_MCU}")
if(CCSDSPACK_MCU_FLAGS)
    message(STATUS "  -DCCSDSPACK_MCU_FLAGS=${CCSDSPACK_MCU_FLAGS}")
endif()

set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/inc")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(GNUInstallDirs)

if(WIN32)
    set(LIBRARY_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib")
    set(BINARY_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
else()
    set(LIBRARY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/lib")
    set(BINARY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")
endif()

set(LIBRARY_SOURCES
    "${SOURCE_DIR}/CCSDSDataField.cpp"
    "${SOURCE_DIR}/CCSDSHeader.cpp"
    "${SOURCE_DIR}/CCSDSManager.cpp"
    "${SOURCE_DIR}/CCSDSTime.cpp"
    "${SOURCE_DIR}/CCSDSPacket.cpp"
    "${SOURCE_DIR}/CCSDSUtils.cpp"
    "${SOURCE_DIR}/CCSDSValidator.cpp"
    "${SOURCE_DIR}/PusSecondaryHeaderFactory.cpp"
    "${SOURCE_DIR}/PusSecondaryHeaders.cpp"
    "${SOURCE_DIR}/PusTailoring.cpp"
)

if(NOT CCSDSPACK_BUILD_MCU)
    list(APPEND LIBRARY_SOURCES "${SOURCE_DIR}/CCSDSConfig.cpp")
endif()

if(CCSDSPACK_BUILD_MCU)
    add_library(${LIB_NAME} STATIC ${LIBRARY_SOURCES})
    add_library(ccsdspack::mcu ALIAS ${LIB_NAME})
else()
    add_library(${LIB_NAME} SHARED ${LIBRARY_SOURCES})
    add_library(ccsdspack::core ALIAS ${LIB_NAME})
endif()

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(${LIB_NAME} PUBLIC cxx_std_17)

if(CCSDSPACK_BUILD_MCU)
    set(CCSDSPACK_MCU_COMPILE_FLAGS "")
    if(CCSDSPACK_MCU_FLAGS)
        separate_arguments(
            CCSDSPACK_MCU_COMPILE_FLAGS
            UNIX_COMMAND
            "${CCSDSPACK_MCU_FLAGS}"
        )
    endif()

    target_compile_options(${LIB_NAME} PRIVATE
        -Os
        -ffunction-sections
        -fdata-sections
        ${CCSDSPACK_MCU_COMPILE_FLAGS}
    )
    target_link_options(${LIB_NAME} PRIVATE -Wl,--gc-sections)
    target_compile_definitions(${LIB_NAME} PUBLIC CCSDS_MCU)
    set_target_properties(${LIB_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE OFF
        OUTPUT_NAME ccsdspack
        ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
    )
else()
    set_target_properties(${LIB_NAME} PROPERTIES
        VERSION ${MAJOR}.${MINOR}.${PATCH}
        SOVERSION ${MAJOR}
        OUTPUT_NAME ccsdspack
        ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}
    )
endif()

if(WIN32)
    set_target_properties(${LIB_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(NOT CCSDSPACK_BUILD_MCU)
    option(ENABLE_TESTER "Build the CCSDSPack tester executable" ON)
    option(ENABLE_ENCODER "Build the CCSDSPack encoder executable" ON)
    option(ENABLE_DECODER "Build the CCSDSPack decoder executable" ON)
    option(ENABLE_VALIDATOR "Build the CCSDSPack validator executable" ON)
    option(CCSDSPACK_ENABLE_FUZZING "Build bounded Clang/libFuzzer robustness targets" OFF)
    message(STATUS "  -DENABLE_TESTER=${ENABLE_TESTER}")
    message(STATUS "  -DENABLE_ENCODER=${ENABLE_ENCODER}")
    message(STATUS "  -DENABLE_DECODER=${ENABLE_DECODER}")
    message(STATUS "  -DENABLE_VALIDATOR=${ENABLE_VALIDATOR}")
    message(STATUS "  -DCCSDSPACK_ENABLE_FUZZING=${CCSDSPACK_ENABLE_FUZZING}")

    if(ENABLE_TESTER)
        include(${CMAKE_SOURCE_DIR}/cmake/tester.cmake)
    endif()
    if(ENABLE_ENCODER)
        include(${CMAKE_SOURCE_DIR}/cmake/encoder.cmake)
    endif()
    if(ENABLE_DECODER)
        include(${CMAKE_SOURCE_DIR}/cmake/decoder.cmake)
    endif()
    if(ENABLE_VALIDATOR)
        include(${CMAKE_SOURCE_DIR}/cmake/validator.cmake)
    endif()
    if(CCSDSPACK_ENABLE_FUZZING)
        include(${CMAKE_SOURCE_DIR}/cmake/fuzz.cmake)
    endif()
endif()

if(CCSDSPACK_BUILD_MCU)
    install(TARGETS ${LIB_NAME}
        EXPORT CCSDSPackTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
else()
    install(TARGETS ${LIB_NAME}
        EXPORT CCSDSPackTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

install(DIRECTORY ${INCLUDE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CCSDSPackConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CCSDSPackConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CCSDSPack"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CCSDSPackConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
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
