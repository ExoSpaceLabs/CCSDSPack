# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(CCSDSPackCustomSecondaryHeaderExample LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  find_package(CCSDSPack 2.0 CONFIG REQUIRED)
  enable_testing()
endif()

add_executable(ccsdspack_example_custom_secondary_header main.cpp)
target_link_libraries(ccsdspack_example_custom_secondary_header PRIVATE ccsdspack::CCSDSPack)
add_test(NAME example_custom_secondary_header COMMAND ccsdspack_example_custom_secondary_header)
