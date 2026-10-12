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
