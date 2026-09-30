# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

set(CCSDSPACK_PACKET_BENCHMARK "CCSDSPack_packet_benchmark")

add_executable(${CCSDSPACK_PACKET_BENCHMARK}
    "${CMAKE_SOURCE_DIR}/test/performance/packet_parse_benchmark.cpp"
)

target_include_directories(${CCSDSPACK_PACKET_BENCHMARK}
    PRIVATE
        "${INCLUDE_DIR}"
)

target_link_libraries(${CCSDSPACK_PACKET_BENCHMARK}
    PRIVATE
        ${LIB_NAME}
)

set_target_properties(${CCSDSPACK_PACKET_BENCHMARK} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(UNIX)
    set_target_properties(${CCSDSPACK_PACKET_BENCHMARK} PROPERTIES
        BUILD_RPATH "${LIBRARY_OUTPUT_DIR}:${CMAKE_BINARY_DIR}/lib"
    )
endif()


set(CCSDSPACK_SERIALIZE_BENCHMARK "CCSDSPack_packet_serialize_benchmark")

add_executable(${CCSDSPACK_SERIALIZE_BENCHMARK}
    "${CMAKE_SOURCE_DIR}/test/performance/packet_serialize_benchmark.cpp"
)

target_include_directories(${CCSDSPACK_SERIALIZE_BENCHMARK}
    PRIVATE
        "${INCLUDE_DIR}"
)

target_link_libraries(${CCSDSPACK_SERIALIZE_BENCHMARK}
    PRIVATE
        ${LIB_NAME}
)

set_target_properties(${CCSDSPACK_SERIALIZE_BENCHMARK} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(UNIX)
    set_target_properties(${CCSDSPACK_SERIALIZE_BENCHMARK} PROPERTIES
        BUILD_RPATH "${LIBRARY_OUTPUT_DIR}:${CMAKE_BINARY_DIR}/lib"
    )
endif()
