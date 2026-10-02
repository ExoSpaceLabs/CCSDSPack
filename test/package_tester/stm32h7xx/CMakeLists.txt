# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)
project(ccsdspack_h755_validation LANGUAGES C CXX)

find_package(CCSDSPack 2.1 CONFIG REQUIRED)
find_package(DAS 0.1 CONFIG REQUIRED)

if(NOT TARGET ccsdspack::CCSDSPack)
    message(FATAL_ERROR "Installed CCSDSPack package does not export ccsdspack::CCSDSPack")
endif()
if(NOT TARGET das::das)
    message(FATAL_ERROR "Installed DAS package does not export das::das")
endif()

set(CCSDSPACK_VALIDATION_SOURCE_SHA "unknown" CACHE STRING
    "CCSDSPack source commit represented by the tested package")
set(CCSDSPACK_VALIDATION_DAS_SHA "unknown" CACHE STRING
    "Pinned DAS commit linked into the test firmware")
set(CCSDSPACK_VALIDATION_PACKAGE_SHA256 "unknown" CACHE STRING
    "SHA-256 of the tested CCSDSPack MCU package")

add_executable(ccsdspack_h755_validation
    CM7/Src/main.cpp
    CM7/Src/syscalls.c)

target_link_libraries(ccsdspack_h755_validation PRIVATE
    ccsdspack::CCSDSPack
    das::das)

target_include_directories(ccsdspack_h755_validation PRIVATE
    "\${CMAKE_CURRENT_SOURCE_DIR}/CM7/Inc"
    "\${CMAKE_CURRENT_SOURCE_DIR}/../hardware")

target_compile_features(ccsdspack_h755_validation PRIVATE
    c_std_11
    cxx_std_17)

target_compile_definitions(ccsdspack_h755_validation PRIVATE
    CCSDS_MCU
    CCSDSPACK_VALIDATION_SOURCE_SHA="\${CCSDSPACK_VALIDATION_SOURCE_SHA}"
    CCSDSPACK_VALIDATION_DAS_SHA="\${CCSDSPACK_VALIDATION_DAS_SHA}"
    CCSDSPACK_VALIDATION_PACKAGE_SHA256="\${CCSDSPACK_VALIDATION_PACKAGE_SHA256}")

target_compile_options(ccsdspack_h755_validation PRIVATE
    -Wall
    -Wextra
    -Werror
    -ffreestanding
    -fno-builtin
    -ffunction-sections
    -fdata-sections
    "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>"
    "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"
    "$<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>")

target_link_options(ccsdspack_h755_validation PRIVATE
    -nostartfiles
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--unresolved-symbols=report-all
    -Wl,-Map=\${CMAKE_CURRENT_BINARY_DIR}/ccsdspack_h755_validation.map)

set_target_properties(ccsdspack_h755_validation PROPERTIES
    SUFFIX ".elf"
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS OFF
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS OFF)
