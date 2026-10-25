# C11 protocol API

[Documentation index](README.md) | [Raw-buffer C++ adapters](RAW_BUFFERS.md) | [Packages](PACKAGES.md)

CCSDSPack v2.1 provides a **standalone C11 protocol core**. It can be built
without a C++ compiler or runtime, and its public interfaces use caller-owned
buffers, views, output capacities, explicit result codes, and fixed-size state.
The C core does not allocate memory internally. That guarantee does **not**
apply to the optional C++17 Packet/Manager ownership layer.

## Use from C

```cmake
find_package(CCSDSPack 2.1 CONFIG REQUIRED)
target_link_libraries(my_c_app PRIVATE ccsdspack::c)
```

```c
#include <ccsdspack/c/ccsdspack.h>

uint8_t bytes[] = {0x01, 0x02, 0x03};
uint16_t crc = 0;
ccsds_status_t status =
    ccsds_crc16_ccitt_false(bytes, sizeof bytes, &crc);
if (status != CCSDS_STATUS_OK) {
    /* Handle the explicit error before using crc. */
}
```

To build only the C target from source:

```bash
CC=gcc CXX=/bin/false cmake -S . -B build-c \
  -DCCSDSPACK_BUILD_CPP=OFF \
  -DCCSDSPACK_BUILD_C_TESTS=ON
cmake --build build-c
ctest --test-dir build-c --output-on-failure
```

The C-only install exports `ccsdspack::c` and the `ccsdspack/c/` headers.
The default hosted build exports the separate `ccsdspack::CCSDSPack` C++17
target as well. See [Packages](PACKAGES.md) for conditional targets.

## Supported C facilities

| Public header under `ccsdspack/c/` | Capabilities |
|---|---|
| `buffer.h`, `bytes.h` | Non-owning buffer views and explicit big-/little-endian integer load/store helpers |
| `error.h` | `ccsds_status_t` values and `ccsds_status_name()` |
| `primary_header.h` | CCSDS primary-header validation, encode/decode, packed fields, and declared packet size |
| `crc.h` | Stateful CRC16 update, parameterized CRC16 calculation, and CRC-16/CCITT-FALSE |
| `packet_view.h` | Bounded, non-owning inspection of one packet and optional CRC16 verification |
| `packet_encode.h` | Checked packet finalization and encoding to supplied output buffers, including two-part data-field input |
| `pus_tc.h`, `pus_tm.h`, `pus.h` | PUS-A/PUS-C TC/TM field codecs and supported tailoring validation |
| `cuc.h` | Basic numeric CUC configuration, encode/decode, and encoded-size calculation |
| `segmentation.h` | Packet-count calculation, segment planning, and modulo-16384 sequence utilities |
| `reassembly.h` | Stateful receive-side application-data reassembly into caller storage |
| `stream.h` | Packet-by-packet stream walking and optional external synchronization-prefix framing |
| `validation.h` | Fixed-capacity named validation checks and sequence/segmentation state |

Include `<ccsdspack/c/ccsdspack.h>` to access the complete public C surface,
or include individual headers as needed.

## Parse one packet without copying

The following complete C11 example inspects a Space Packet containing three
application-data octets. The example explicitly selects **no packet-level CRC**;
for CRC16 streams, use `CCSDS_PACKET_ERROR_CONTROL_CRC16` and the matching
CRC configuration (`NULL` selects the default CRC-16/CCITT-FALSE parameters).

```c
#include <stddef.h>
#include <stdint.h>
#include <ccsdspack/c/ccsdspack.h>

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

    if (view.consumed != sizeof(wire) ||
        view.primary_header.apid != 0x0123U ||
        view.data_field.size != 3U ||
        view.data_field.data != wire + CCSDS_PRIMARY_HEADER_SIZE) return 3;
    return 0;
}
```

`ccsds_packet_declared_size()` inspects the primary header's declared size,
not the full body. The bounded packet-view parser checks packet boundaries,
version-zero policy, the selected packet-level error-control mode and
structural Idle Packet constraints. It does **not** interpret or validate
custom/PUS secondary-header fields; applications use the dedicated PUS codecs
or a configured C++ Packet adapter for that work.

All buffer views in `ccsds_packet_view_t` alias the caller's input, so they
must not outlive the input storage or be used after it changes.

## Unified packet validation for contiguous and split inputs

The C11 core is the authority for **wire-packet policy**. 
`ccsds_packet_view_parse()` retains its existing contiguous-buffer API and
permits trailing stream bytes. The new `ccsds_packet_parts_parse()` consumes
a separate, **exact six-byte primary-header span and exact body span** without
joining them into a temporary buffer. CRC16 is checked incrementally across
the two spans, with configured polynomial, initial and final XOR values.
Returned `ccsds_packet_parts_view_t` fields alias their respective
caller-owned spans. Unlike a contiguous packet view, no whole-packet pointer
is exposed. All output views remain untouched when validation fails.

`ccsds_packet_view_parse_ex()` adds optional structured
`ccsds_packet_parse_reason_t` diagnostics to the established contiguous API.
Both parsers share one C11 implementation for version-zero, Packet Data
Length, Idle Packet, CRC trailer and checksum policy. An optional reason output
preserves detailed diagnostics for the C++ adapter, which still owns object
lifetime, application-data vectors and custom secondary-header parsing.
The original `ccsds_packet_view_parse()` API remains source-compatible.

## Incremental stream reception

For chunked UART/DMA input and sync recovery, the allocation-free
`ccsds_stream_receiver_t` uses caller-owned scratch storage and returns
validated, non-owning packet views. See the [receiver design and example](STREAM_RECEIVER.md).
The existing `ccsds_packet_stream_t` still walks already-contiguous buffers.

## Ownership and boundaries

- Packet views alias input memory: retain the backing buffer for as long as
  those views are used.
- Encode/reassembly functions write into buffers supplied by the caller and
  report errors for invalid sizes/capacities rather than allocating storage.
- The byte-order helpers themselves operate on supplied pointers and require
  callers to ensure the referenced buffer contains enough bytes.
- PUS support covers the documented **secondary-header layouts**, not PUS
  service implementations. Packet error control is optional CCSDSPack policy
  inside the Packet Data Field, not a third CCSDS Space Packet field.
- CUC operates on numeric coarse/fine values; calendar/UTC/leap-second
  conversion and mission time correlation are outside scope.
- The standalone C API does not expose hosted configuration files, CLI tools,
  or the owning C++ `Packet`/`Manager` classes.

See [Space Packet profile](CCSDS_133_0_B_2_PROFILE.md),
[PUS/CUC scope](PUS_CUC_COMPLIANCE.md), and
[Structured validation](VALIDATION.md) for the corresponding constraints and
conformance evidence.
