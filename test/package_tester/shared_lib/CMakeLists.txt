# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13)
project(ccsdspack_installed_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This project must resolve CCSDSPack exclusively through its installed CMake
# package. CI passes CMAKE_PREFIX_PATH to the installation under test.
find_package(CCSDSPack CONFIG REQUIRED)

add_executable(hello_ccsds src/main.cpp)
target_link_libraries(hello_ccsds PRIVATE ccsdspack::CCSDSPack)

enable_testing()
add_test(NAME installed_shared_library_consumer COMMAND hello_ccsds)
