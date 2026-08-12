# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)
project(CCSDSPackHardwareAcceptance LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CCSDSPack 2.0 CONFIG REQUIRED)

add_executable(ccsdspack_hardware_acceptance main.cpp)
target_include_directories(ccsdspack_hardware_acceptance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ccsdspack_hardware_acceptance PRIVATE CCSDSPack::ccsdspack)

enable_testing()
add_test(NAME ccsdspack_hardware_acceptance COMMAND ccsdspack_hardware_acceptance)
