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

The v2 implementation retains the validated v1.2 six-octet CCSDS primary-header and Packet Data Field behaviour and adds explicit mission profiles, standards-oriented PUS-A/PUS-C secondary headers, CUC time support, checked finalization, and structured validation.

CCSDSPack v2 enforces:

- Packet Version Number `000`;
- telemetry and telecommand Packet Types;
- the complete 11-bit APID range;
- Idle Packet structural rules for APID `0x7FF`;
- Packet Data Length as the number of octets after the primary header minus one;
- CCSDS sequence flags and modulo-16384 Packet Sequence Count handling;
- explicit packet-error-control selection;
- explicit PUS revision/direction/profile consistency.

The Packet Data Field can contain 1 through 65,536 octets, giving a total serialized packet size of 7 through 65,542 octets.

Use:

```cpp
std::size_t packetSize = packet.getSerializedSize();
```

for the complete size range of an already constructed packet.

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

A complete raw buffer can then be parsed without requiring the caller to first construct a vector:

```cpp
ccsds::Packet packet;
packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet, rxBuffer, receivedBytes);
if (!consumed) return consumed.error().code();
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
- concatenated packet-stream handling;
- optional project-specific CRC16 trailer;
- complete 11-bit APID handling and Idle Packet validation;
- modulo-16384 sequence counting and segmentation utilities;
- one configured Packet Identification value per `ccsds::Manager` instance;
- low-copy const-reference Manager inspection;
- custom and opaque secondary-header support;
- PUS-A and PUS-C direction-specific TC/TM secondary headers;
- separate fixed PUS and extensible custom secondary-header factories;
- explicit mission-profile validation for revision, direction, identifiers, time, spare fields, and packet error control;
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

The host-side `ccsds_validator` executable delegates protocol/profile checks to this library implementation rather than maintaining separate validation logic.

See [Structured validation](docs/VALIDATION.md).

## PUS secondary headers

PUS selection is explicit and direction-safe. Canonical selectors are:

- `PUS:revA:TC` and `PUS:revA:TM`;
- `PUS:revC:TC` and `PUS:revC:TM`.

Custom headers remain string-keyed in `ccsds::SecondaryHeaderFactory`; the reserved PUS selectors are owned by the non-extensible `ccsds::pus::SecondaryHeaderFactory`.

```cpp
auto profile = ccsds::pus::makeProfile(
  ccsds::pus::Revision::C,
  ccsds::pus::Direction::Telecommand);

ccsds::Packet packet;
const auto primary = packet.setPrimaryHeader(
  {0, 1, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});
if (!primary) return primary.error().code();

const auto profileResult = packet.setMissionProfile(profile);
if (!profileResult) return profileResult.error().code();

const auto secondary = packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TcHeader>(
    profile, 17, 1, 0x1234, 0x09));
if (!secondary) return secondary.error().code();

const auto data = packet.setApplicationData({0x10, 0x20});
if (!data) return data.error().code();

const auto wire = packet.serialize();
if (!wire) return wire.error().code();
```

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

The [`example/`](example/README.md) directory contains independent installed-package consumers and complete generic/PUS configuration profiles. Build all examples, or select one by directory name:

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
- [PUS and mission tailoring](docs/MISSION_TAILORING.md)
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
- explicit PUS-A/PUS-C revision and direction types;
- explicit mission profiles and numeric CUC time;
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
