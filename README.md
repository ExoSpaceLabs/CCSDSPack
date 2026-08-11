<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

<div style="text-align: center;">
    <img alt="CCSDSPack logo" src="docs/imgs/Logo.png" width="400" />
</div>

# CCSDSPack [[ExoSpaceLabs](https://github.com/ExoSpaceLabs)]

**CCSDSPack** is a C++17 library for creating, parsing, managing, and validating CCSDS Space Packet protocol data units.

The v2 packet layer targets **CCSDS 133.0-B-2, Issue 2, including Editorial Change 2 (September 2024)** and provides standards-facing secondary headers for **ECSS-E-70-41A (PUS-A)** and **ECSS-E-ST-70-41C (PUS-C)**.

> [!IMPORTANT]
> The implementation scope is the documented Space Packet PDU profile and supported PUS secondary-header layouts. CCSDSPack does not implement every PUS service, the complete abstract Packet Service, transfer frames, or a complete protocol entity.

> [!IMPORTANT]
> v2 removed the project-specific `PusA`, `PusB`, and `PusC` model. The public PUS types are `ccsds::pus::rev_a::TcHeader`, `ccsds::pus::rev_a::TmHeader`, `ccsds::pus::rev_c::TcHeader`, and `ccsds::pus::rev_c::TmHeader`. There is no standards-facing PUS-B revision.

## Status

| Linux | Windows |
|---|---|
| ![Linux build status](https://img.shields.io/github/actions/workflow/status/ExoSpaceLabs/CCSDSPack/linux.yml?branch=develop) | ![Windows build status](https://img.shields.io/github/actions/workflow/status/ExoSpaceLabs/CCSDSPack/windows.yml?branch=develop) |

| Platform | CI |
|---|---|
| Ubuntu 22.04 | ![Ubuntu 22.04](https://github.com/ExoSpaceLabs/CCSDSPack/actions/workflows/linux.yml/badge.svg?job=ubuntu-22-04) |
| Ubuntu 24.04 | ![Ubuntu 24.04](https://github.com/ExoSpaceLabs/CCSDSPack/actions/workflows/linux.yml/badge.svg?job=ubuntu-24-04) |
| Ubuntu latest | ![Ubuntu latest](https://github.com/ExoSpaceLabs/CCSDSPack/actions/workflows/linux.yml/badge.svg?job=ubuntu-latest) |
| Windows latest | ![Windows latest](https://github.com/ExoSpaceLabs/CCSDSPack/actions/workflows/windows.yml/badge.svg?job=windows-latest) |

UML generation is currently **manual-only** and is not a v2.0.0 CI/release gate. The workflow remains available through `workflow_dispatch` for later documentation maintenance.

## v2 Space Packet PDU profile

The v2 implementation retains the validated v1.2 six-octet CCSDS primary-header and Packet Data Field behaviour and adds standards-oriented PUS-A/PUS-C secondary headers, numeric CUC time support, checked finalization, structured validation, and buffer-oriented parsing.

The Packet Data Field contains optional secondary-header bytes, mission application data, and, when enabled, the optional CCSDSPack CRC16 trailer.

### CCSDSPack packet layout

The following diagram represents the generic Space Packet wire layout.

![CCSDSPack Space Packet layout](docs/imgs/ccsdsPacket.drawio.png)

When the optional CCSDSPack CRC16 packet-error-control mode is enabled, the final two Packet Data Field octets are reserved for the CRC trailer. CCSDS 133.0-B-2 does not define this trailer as a third top-level Space Packet structural field.

Detailed scope, limitations, and migration behavior are documented in the [CCSDS 133.0-B-2 EC2 Space Packet PDU profile](docs/CCSDS_133_0_B_2_PROFILE.md).

### Primary-header rules

CCSDSPack v2 enforces:

- Packet Version Number `000`;
- telemetry and telecommand Packet Types;
- the complete 11-bit APID range;
- Idle Packet structural rules for APID `0x7FF`;
- Packet Data Length as the number of octets after the primary header minus one;
- CCSDS sequence flags and modulo-16384 Packet Sequence Count handling;
- explicit packet-level error-control selection;
- consistency between a directional secondary header and the CCSDS Packet Type.

`ccsds::PacketDirection` is the generic direction type. A direction-neutral custom header leaves Packet Type under caller control. A concrete PUS `TcHeader` or `TmHeader` reports its direction intrinsically and `Packet::setSecondaryHeader()` synchronizes Packet Type automatically.

CCSDSPack uses Packet Sequence Count semantics for both telemetry and telecommand packets. The optional telecommand Packet Name interpretation is not implemented.

### Packet size

The Packet Data Field can contain 1 through 65,536 octets, giving a total serialized packet size of 7 through 65,542 octets.

CRC16 mode has a practical minimum serialized packet size of 8 octets.

```cpp
std::size_t packetSize = packet.getSerializedSize();
```

The legacy `getFullPacketLength()` API returns `std::uint16_t` and saturates at `UINT16_MAX` rather than wrapping.

### Packet error control

`ccsds::PacketErrorControlMode` is generic packet-level policy and is independent of PUS/custom secondary headers:

- `PacketErrorControlMode::CRC16`, the existing default;
- `PacketErrorControlMode::None`.

```cpp
packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
```

In CRC16 mode, CCSDSPack reserves the final two Packet Data Field octets for CRC-16/CCITT-FALSE and includes those octets in Packet Data Length. There is no separate CRC owned by a PUS secondary header. A receiver must select the expected packet error-control mode before parsing.

### Library architecture

The following diagram shows the main relationships between the user application, `ccsds::Manager`, `ccsds::Packet`, primary-header handling, Packet Data Field handling, secondary-header creation, serialization, validation, CRC16, and utility functions.

![CCSDSPack library architecture](docs/imgs/CCSDSPack_architecture.drawio.png)

### Raw transport buffers

The existing `std::vector<std::uint8_t>` APIs remain the easiest general-purpose interface. v2 also provides additive pointer-plus-size APIs for transport-owned buffers such as UART, DMA, SpaceWire, TCP, or shared-memory receive areas.

A receiver can determine the complete packet size from only the six-byte primary header:

```cpp
std::uint8_t primaryHeader[6];
receive(primaryHeader, sizeof(primaryHeader));

const auto packetSize = ccsds::buffer::declaredPacketSize(
  primaryHeader, sizeof(primaryHeader));
if (!packetSize) return packetSize.error().code();

// Receive packetSize.value() - 6 more bytes from the transport.
```

A complete generic raw buffer can then be parsed without requiring the caller to first construct a vector:

```cpp
ccsds::Packet packet;
packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet, rxBuffer, receivedBytes);
if (!consumed) return consumed.error().code();
```

Typed raw PUS parsing uses the same concrete-header model as vector parsing:

```cpp
ccsds::Packet packet;
const auto consumed =
  ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TcHeader>(
    packet, rxBuffer, receivedBytes);
```

`ccsds::Manager` also has raw pointer + size overloads for application data, one-packet ingestion, and concatenated packet streams:

```cpp
manager.setApplicationData(payload, payloadSize);
manager.addPacketFromBuffer(packetBytes, packetSize);
manager.load(streamBytes, streamSize);
```

These signatures intentionally coexist with the vector APIs. In v2.0 the raw parsing adapters still bridge to the existing vector-backed implementation internally. That keeps the public API useful now while leaving room for later zero-copy or heap-free internals without forcing callers to change again.

For low-copy inspection, a const Manager exposes references to its template, packet collection, and Validator through `getTemplateReference()`, `getPacketsReference()`, and `getValidatorReference()`.

See [Raw-buffer APIs](docs/RAW_BUFFERS.md).

## Features

- CCSDS 133.0-B-2 EC2 Space Packet PDU construction and parsing;
- exact bounded parsing with consumed-byte reporting;
- raw pointer + size transport adapters and primary-header packet-size inspection;
- typed PUS vector/raw parsing;
- concatenated packet-stream handling;
- optional project-specific packet-level CRC16 trailer;
- complete 11-bit APID handling and Idle Packet validation;
- modulo-16384 sequence counting and segmentation utilities;
- one complete Packet template and sequence stream per `ccsds::Manager`;
- low-copy const-reference Manager inspection;
- custom and opaque secondary-header support;
- intrinsic PUS-A/PUS-C TC/TM secondary-header identity;
- optional direction-specific PUS tailoring;
- separate fixed PUS and extensible custom secondary-header factories;
- numeric basic CUC time with explicit epoch, P-field, and coarse/fine-width policy;
- fixed-capacity structured `ccsds::ValidationReport` with named generic/PUS checks;
- symbolic `errorCodeName()` and `validationCodeName()` diagnostics;
- exception-free `Result` and `Error` handling;
- C++17 hosted and bare-metal builds;
- MCU builds compatible with `-fno-exceptions -fno-rtti`;
- encoder, decoder, validator, and regression-test executables on hosted platforms;
- installed CMake package and shared-library consumer tests.

## Structured validation

`ccsds::Validator` validates an existing packet without mutating it. Reports use named `ValidationCode` values instead of positional boolean indices:

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);

if (report.failed(ccsds::ValidationCode::PacketDataLength)) {
  handleLengthFailure();
}

if (report.failed(ccsds::ValidationCode::PusDirection)) {
  handlePusDirectionFailure();
}
```

`ValidationReport` stores up to 32 performed checks in `std::array` and performs no dynamic allocation itself. The API is available in `CCSDS_MCU`, requires C++17, and does not require RTTI or exceptions.

PUS validation reads revision, direction, and tailoring from the installed concrete PUS secondary header. Packet-level CRC/PEC and the secondary-header contract are validated independently. The host-side `ccsds_validator` executable delegates these checks to the library implementation.

See [Structured validation](docs/VALIDATION.md).

## PUS secondary headers

PUS revision and direction are properties of the concrete type:

```text
ccsds::pus::rev_a::TcHeader -> PUS-A + Telecommand
ccsds::pus::rev_a::TmHeader -> PUS-A + Telemetry
ccsds::pus::rev_c::TcHeader -> PUS-C + Telecommand
ccsds::pus::rev_c::TmHeader -> PUS-C + Telemetry
```

The common direction type is `ccsds::PacketDirection`. A PUS header cannot be constructed with a contradictory direction or revision.

### PUS-C TC with default tailoring

No profile object is required:

```cpp
ccsds::Packet packet;

const auto primary = packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});
if (!primary) return primary.error().code();

const auto secondary = packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TcHeader>(
    17,       // service type
    1,        // service subtype
    0x1234,   // source ID
    0x09));   // acknowledgement flags
if (!secondary) return secondary.error().code();

// Packet Type is now Telecommand because TcHeader is directional.

const auto data = packet.setApplicationData({0x10, 0x20});
if (!data) return data.error().code();

const auto wire = packet.serialize();
if (!wire) return wire.error().code();
```

PUS-C identifier widths fixed by the supported standard profile are not exposed as arbitrary tuning knobs: TC source ID and TM destination ID use two octets.

### Optional PUS tailoring

Tailoring is supplied only when the selected PUS layout has optional mission choices. For example, a PUS-C telemetry timestamp:

```cpp
ccsds::pus::rev_c::TmTailoring tailoring;
tailoring.timestampPresent = true;
tailoring.cuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Implicit,
  4,
  0
};

auto header = std::make_shared<ccsds::pus::rev_c::TmHeader>(
  tailoring,
  3,          // service type
  25,         // service subtype
  7,          // message-type counter
  0x1234,     // destination ID
  0,          // time-reference status
  ccsds::time::CucTime{0x01020304, 0});

packet.setSecondaryHeader(header);
```

PUS-A exposes its revision-specific optional identifier widths and TM packet-subcounter choice through `rev_a::TcTailoring` and `rev_a::TmTailoring`.

### PUS deserialization

A preinstalled empty/default header can act as the parsing schema:

```cpp
ccsds::Packet packet;
packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>());

const auto result = packet.deserialize(wire);
```

Or use the typed convenience form:

```cpp
ccsds::Packet packet;
const auto result =
  packet.deserialize<ccsds::pus::rev_c::TmHeader>(wire);
```

When tailoring is required, pass it to the typed constructor path:

```cpp
const auto result =
  packet.deserialize<ccsds::pus::rev_c::TmHeader>(
    wire, tailoring);
```

The parser reads the CCSDS primary-header Packet Type and rejects a mismatch with the concrete header direction. Canonical runtime selectors remain available for configuration/dynamic use:

- `PUS:revA:TC` and `PUS:revA:TM`;
- `PUS:revC:TC` and `PUS:revC:TM`.

Custom headers remain string-keyed in `ccsds::SecondaryHeaderFactory`; the reserved PUS selectors are owned by the non-extensible `ccsds::pus::SecondaryHeaderFactory`.

## Manager templates

`ccsds::Manager` does not maintain a second profile object. Its complete `ccsds::Packet` template is the generation and receive contract:

```cpp
ccsds::Packet packetTemplate;
packetTemplate.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>(tailoring));

ccsds::Manager manager;
manager.setPacketTemplate(packetTemplate);
manager.load(streamBytes);
```

The template therefore carries Packet Identification, packet-level PEC mode, secondary-header type, PUS revision/direction, and any PUS tailoring in one place.

## Build from source

### Requirements

- CMake 3.16 or newer;
- a C++17 compiler;
- GCC 8.5 or newer on supported Linux configurations.

```bash
git clone https://github.com/ExoSpaceLabs/CCSDSPack.git
cd CCSDSPack
cmake -S . -B build
cmake --build build
```

Run the regression tester from the repository binary directory:

```bash
./bin/CCSDSPack_tester
```

Install the library and exported package:

```bash
cmake --install build
```

### Main CMake options

| CMake option | Default | Description |
|---|---:|---|
| `CCSDSPACK_BUILD_MCU` | `OFF` | Build the bare-metal static-library profile |
| `CCSDSPACK_MCU_FLAGS` | empty | Additional flags for the MCU library target |
| `ENABLE_TESTER` | `ON` | Build `CCSDSPack_tester` |
| `ENABLE_ENCODER` | `ON` | Build `ccsds_encoder` |
| `ENABLE_DECODER` | `ON` | Build `ccsds_decoder` |
| `ENABLE_VALIDATOR` | `ON` | Build `ccsds_validator` |

### Windows with MinGW

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

The Windows workflow copies the shared-library DLL beside the test, example, and external-consumer executables before running them.

### Bare-metal Cortex-M

```bash
cmake -S . -B build-mcu \
  -DCCSDSPACK_BUILD_MCU=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DCCSDSPACK_MCU_FLAGS="-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb"
cmake --build build-mcu -j
```

Host-only `ccsds::Config` and CLI executables are excluded from the MCU target. Packet, Manager, raw-buffer adapters, PUS, CUC, Result, and Validator APIs remain available to the static-library consumer.

## CMake integration

After installing CCSDSPack:

```cmake
find_package(CCSDSPack 2.0 CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ccsdspack::CCSDSPack)
target_compile_features(my_app PRIVATE cxx_std_17)
```

For a non-standard install prefix:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/install/prefix
```

The [`example/`](example/README.md) directory contains independent installed-package consumers and complete generic/PUS configurations. Build all examples, or select one by directory name:

```bash
./example/build_examples.sh all /path/to/install/prefix
./example/build_examples.sh raw_buffer_packet /path/to/install/prefix
./example/build_examples.sh raw_buffer_manager /path/to/install/prefix
./example/build_examples.sh pus_c_telecommand /path/to/install/prefix
```

## Command-line tools

Hosted builds can provide:

- `ccsds_encoder`;
- `ccsds_decoder`;
- `ccsds_validator`;
- `CCSDSPack_tester`.

See [CLI reference](docs/CLI.md).

## Documentation

- [Documentation index](docs/README.md)
- [Raw-buffer APIs](docs/RAW_BUFFERS.md)
- [Structured validation](docs/VALIDATION.md)
- [CCSDS compliance baseline](docs/CCSDS_COMPLIANCE.md)
- [CCSDS 133.0-B-2 EC2 PDU profile](docs/CCSDS_133_0_B_2_PROFILE.md)
- [PUS tailoring](docs/MISSION_TAILORING.md)
- [v1 to v2 migration](docs/MIGRATION_V1_TO_V2.md)
- [CLI reference](docs/CLI.md)
- [Configuration reference](docs/CONFIG.md)
- [Error handling](docs/ERROR.md)
- [Packages](docs/PACKAGES.md)
- [Cross-build guide](docs/CROSSBUILD.md)
- [Examples](docs/EXAMPLES.md)
- [Generated API reference](https://exospacelabs.github.io/CCSDSPack/html/)

## Compatibility

v2 is a breaking **source/API and PUS-layout** release. The generic Space Packet wire corrections introduced by v1.2.0 are retained rather than being reintroduced as v2 changes.

The principal v1.2-to-v2 changes include:

- namespace `CCSDS` to `ccsds`;
- removal of project-specific `PusA`, `PusB`, and `PusC`;
- intrinsic PUS-A/PUS-C TC/TM concrete types;
- removal of the intermediate unreleased `MissionProfile` API in favour of generic Packet policy plus optional PUS tailoring;
- generic `ccsds::PacketDirection`;
- numeric CUC time;
- secondary-header API renaming;
- checked serialization/finalization results;
- structured named Validator reports;
- raw pointer + size transport adapters alongside vector convenience APIs;
- v2 configuration schema and canonical PUS selectors.

See [v1 to v2 migration](docs/MIGRATION_V1_TO_V2.md).

## Packages and container

Release packages are published under GitHub Releases. Container images use:

```bash
docker pull ghcr.io/exospacelabs/ccsdspack:<version>
```

## License

CCSDSPack is licensed under the Apache License 2.0. See [LICENSE](LICENSE) and [Notice.md](Notice.md).
