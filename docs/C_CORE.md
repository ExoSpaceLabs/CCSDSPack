<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Native C11 protocol API

[Documentation index](README.md) | [Space Packet PDU profile](CCSDS_133_0_B_2_PROFILE.md) | [Raw-buffer C++ adapters](RAW_BUFFERS.md)

CCSDSPack v2.1.0 exports an independently usable C11 protocol core. Include
`<ccsdspack/c/ccsdspack.h>` or individual headers under
`<ccsdspack/c/>`. The C API uses explicit `ccsds_status_t` results and
caller-supplied buffers. It does not depend on `std::vector`, a C++ compiler,
exceptions, RTTI, or implicit heap allocation.

## Build and link

For an installed package:

```cmake
cmake_minimum_required(VERSION 3.16)
project(packet_receiver LANGUAGES C)
find_package(CCSDSPack 2.1 CONFIG REQUIRED)
add_executable(packet_receiver main.c)
target_link_libraries(packet_receiver PRIVATE ccsdspack::c)
```

For a C-only CCSDSPack source build:

```bash
CC=gcc CXX=/bin/false cmake -S . -B build-c \
  -DCCSDSPACK_BUILD_CPP=OFF \
  -DCCSDSPACK_BUILD_C_TESTS=ON
cmake --build build-c
ctest --test-dir build-c --output-on-failure
```

The existing C-only installed-package consumer is maintained at
`test/package_tester/c_core/main.c`; it exercises primary headers, CUC,
packet views, PUS TC/TM, segmentation, reassembly, and validation.

## Read a packet from caller-owned storage

This complete example uses a version-zero packet with three application-data
octets and **no** packet-level CRC trailer:

```c
#include <stddef.h>
#include <stdint.h>
#include "ccsdspack/c/ccsdspack.h"

int main(void) {
    const uint8_t wire[] = {
        0x01, 0x23, 0xC0, 0x2A, 0x00, 0x02, 0xAA, 0xBB, 0xCC
    };
    size_t declared = 0U;
    ccsds_packet_view_t view;

    ccsds_status_t status =
        ccsds_packet_declared_size(wire, sizeof(wire), &declared);
    if (status != CCSDS_STATUS_OK || declared != sizeof(wire)) return 1;

    status = ccsds_packet_view_parse(
        wire, sizeof(wire), CCSDS_PACKET_ERROR_CONTROL_NONE,
        NULL, &view);
    if (status != CCSDS_STATUS_OK) return 2;
    if (view.consumed != sizeof(wire)
        || view.primary_header.apid != 0x0123U
        || view.data_field.size != 3U
        || view.data_field.data != wire + CCSDS_PRIMARY_HEADER_SIZE) return 3;
    return 0;
}
```

`ccsds_packet_declared_size()` inspects just the first six octets.
`ccsds_packet_view_parse()` validates the complete declared packet,
selected packet-level CRC mode, version-zero policy, and structural Idle Packet
constraints. With CRC16 selected, supply the appropriate CRC configuration or
`NULL` for the default CCITT-FALSE parameters.

The `ccsds_packet_view_t` fields **alias the original input**. The caller
must keep that buffer alive and unchanged while using the views. This function
does not interpret or validate an application-specific/custom/PUS secondary
header's fields. Use the dedicated `pus_tc.h` / `pus_tm.h` codecs, or a
configured C++ `Packet` adapter, for that schema-level work.

## Packet construction, streams, and application-data handling

| Header | Implemented capability |
|---|---|
| `primary_header.h` | decode/encode and field-width checks, plus version-zero packet declared-size inspection |
| `packet_encode.h` | checked Packet Data Length/CRC finalization and caller-buffer encoding; one- or two-span data fields |
| `crc.h` / `bytes.h` | incremental CRC16 and explicit byte-order helpers |
| `packet_view.h` | non-owning bounded packet view and CRC policy |
| `stream.h` | bounded packet cursor, optional four-byte external sync prefix |
| `segmentation.h` | packet-count calculation, offset/size/flag planning, 14-bit sequence rollover |
| `reassembly.h` | caller-buffer application-data accumulation with optional sequence validation |
| `pus_tc.h` / `pus_tm.h` | PUS-A and PUS-C TC/TM codecs and tailoring |
| `cuc.h` | basic numeric CUC encode/decode and layout validation |
| `validation.h` | named fixed-capacity validation reports and sequence state |
| `error.h` | explicit `ccsds_status_t` values and `ccsds_status_name()` |

For example, `ccsds_packet_stream_init()` and
`ccsds_packet_stream_next()` walk successive packet views without copies.
`ccsds_segmentation_plan()` chooses the caller's next payload slice and
sequence flag; `ccsds_reassembly_accept()` appends that packet's application
data into a buffer supplied by the caller. The library does not own network
transport, DMA lifetimes, or mission buffering.

**Policy boundary:** `ccsds_primary_header_validate()` validates the widths
of the logical primary-header fields, and does *not* impose version zero.
Packet-level operations such as `ccsds_packet_declared_size()`,
`ccsds_packet_view_parse()`, and checked finalization apply the supported
Space Packet version-zero policy.

## Errors and ownership

A C function normally returns `CCSDS_STATUS_OK` or a named error; output
parameters are written only as documented for that function. Unlike the C++
`Result<T>`, the C API reports detailed buffer, CUC, and PUS errors using
dedicated `CCSDS_STATUS_*` values. Call `ccsds_status_name(status)` to
obtain its symbolic name.

The allocation-free guarantees apply to the documented C primitives.
The optional C++17 Packet/Manager layer owns state and may allocate. Neither
layer supplies complete PUS services, lower-layer transfer frames, or
automatic mission time correlation.
