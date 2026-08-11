# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)
project(CCSDSPackExamples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CCSDSPack 2.0 CONFIG REQUIRED)

enable_testing()
add_subdirectory(basic_packet)
add_subdirectory(custom_secondary_header)
add_subdirectory(pus_c_telecommand)
add_subdirectory(pus_c_telemetry)
add_subdirectory(raw_buffer_packet)
add_subdirectory(raw_buffer_manager)
