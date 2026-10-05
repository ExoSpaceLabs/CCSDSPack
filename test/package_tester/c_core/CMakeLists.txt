# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)
project(CCSDSPackCInstalledConsumer LANGUAGES C)

find_package(CCSDSPack 2.1 CONFIG REQUIRED)

add_executable(ccsdspack_c_consumer main.c)
target_link_libraries(ccsdspack_c_consumer PRIVATE ccsdspack::c)

enable_testing()
add_test(NAME ccsdspack_c_consumer COMMAND ccsdspack_c_consumer)
