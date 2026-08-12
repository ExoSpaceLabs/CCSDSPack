# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "CCSDSPACK_ENABLE_FUZZING requires Clang/libFuzzer")
endif()

set(CCSDSPACK_FUZZ_DIR "${CMAKE_SOURCE_DIR}/test/fuzz")
set(CCSDSPACK_FUZZ_SANITIZERS "-fsanitize=fuzzer,address,undefined")

set(CCSDSPACK_FUZZ_TARGETS
    ccsdspack_fuzz_declared_packet_size
    ccsdspack_fuzz_packet
    ccsdspack_fuzz_pus
    ccsdspack_fuzz_cuc
)

add_executable(ccsdspack_fuzz_declared_packet_size
    "${CCSDSPACK_FUZZ_DIR}/fuzz_declared_packet_size.cpp")
add_executable(ccsdspack_fuzz_packet
    "${CCSDSPACK_FUZZ_DIR}/fuzz_packet.cpp")
add_executable(ccsdspack_fuzz_pus
    "${CCSDSPACK_FUZZ_DIR}/fuzz_pus.cpp")
add_executable(ccsdspack_fuzz_cuc
    "${CCSDSPACK_FUZZ_DIR}/fuzz_cuc.cpp")

foreach(target IN LISTS CCSDSPACK_FUZZ_TARGETS)
    target_include_directories(${target} PRIVATE "${INCLUDE_DIR}")
    target_link_libraries(${target} PRIVATE ${LIB_NAME})
    target_compile_options(${target} PRIVATE
        ${CCSDSPACK_FUZZ_SANITIZERS}
        -fno-omit-frame-pointer
    )
    target_link_options(${target} PRIVATE
        ${CCSDSPACK_FUZZ_SANITIZERS}
        -fno-omit-frame-pointer
    )
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fuzz"
    )
    if(UNIX)
        set_target_properties(${target} PROPERTIES
            BUILD_RPATH "${LIBRARY_OUTPUT_DIR}:${CMAKE_BINARY_DIR}/lib"
        )
    endif()
endforeach()
