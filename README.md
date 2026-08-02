<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

<div style="text-align: center;">
    <img alt="CCSDSPack logo" src="docs/imgs/Logo.png" width="400" />
</div>

# CCSDSPack [[ExoSpaceLabs](https://github.com/ExoSpaceLabs)]

**CCSDSPack** is a C++17 library for creating, parsing, managing, and validating CCSDS Space Packet protocol data units.

The v1.2 packet layer targets **CCSDS 133.0-B-2, Issue 2, including Editorial Change 2 (September 2024)** through a documented Space Packet PDU profile.

> [!IMPORTANT]
> The v1.2 implementation scope is limited to the documented **Space Packet PDU profile**. CCSDSPack does not implement the complete abstract Packet Service, Octet String Service, all protocol procedures, or a Protocol Implementation Conformance Statement.

> [!IMPORTANT]
> The bundled `PusA`, `PusB`, and `PusC` classes are legacy project-specific secondary-header formats. They are not official ECSS Packet Utilisation Standard implementations.

## Status

| Linux | Windows |
|---|---|
| ![Linux build status](https://img.shields.io/github/actions/workflow/status/ExoSpaceLabs/CCSDSPack/linux.yml?branch=main) | ![Windows build status](https://img.shields.io/github/actions/workflow/status/ExoSpaceLabs/CCSDSPack/windows.yml?branch=main) |

| Platform | CI |
|---|---|
| Ubuntu 22.04 | ![Ubuntu 22.04](https://github.com/ExoSpaceLabs/CCSDSPack/actions/workflows/linux.yml/badge.svg?job=ubuntu-22-04) |
| Ubuntu 24.04 | ![Ubuntu 24.04](https://github.com/ExoSpaceLabs/CCSDSPack/actions/workflows/linux.yml/badge.svg?job=ubuntu-24-04) |
| Ubuntu latest | ![Ubuntu latest](https://github.com/ExoSpaceLabs/CCSDSPack/actions/workflows/linux.yml/badge.svg?job=ubuntu-latest) |
| Windows latest | ![Windows latest](https://github.com/ExoSpaceLabs/CCSDSPack/actions/workflows/windows.yml/badge.svg?job=windows-latest) |

## v1.2 Space Packet PDU profile

The v1.x implementation serializes packets using a six-octet CCSDS primary header followed by the Packet Data Field.

The Packet Data Field contains optional secondary-header bytes, mission application data, and, when enabled, the optional CCSDSPack CRC16 trailer.

### CCSDSPack v1.2 packet layout

The following diagram represents the wire layout produced by the v1.2 implementation.

![CCSDSPack v1.2 Space Packet layout](docs/imgs/ccsdsPacket.drawio.png)

When the optional CCSDSPack CRC16 profile is enabled, the final two Packet Data Field octets are reserved for the CRC trailer. CCSDS 133.0-B-2 does not define this trailer as a third top-level Space Packet structural field.

Detailed scope, limitations, and migration behavior are documented in the [CCSDS 133.0-B-2 EC2 Space Packet PDU profile](docs/CCSDS_133_0_B_2_PROFILE.md).

### Primary-header rules

CCSDSPack v1.2 enforces the following packet-level rules:

- Packet Version Number is `000`;
- telemetry and telecommand Packet Types are supported;
- the complete 11-bit APID range is supported;
- APID `0x7FF` is reserved for Idle Packets;
- within the CCSDSPack v1.2 profile, Idle Packets omit the secondary header and carry mission-defined idle data;
- Packet Data Length is the number of octets after the primary header minus one;
- Sequence Flags use the CCSDS first, continuation, last, and unsegmented values;
- Packet Sequence Count advances modulo 16384 in automatic `CCSDS::Manager` mode.

CCSDSPack uses Packet Sequence Count semantics for both telemetry and telecommand packets. The optional telecommand Packet Name interpretation is not implemented.

### Packet size

The Packet Data Field can contain 1 through 65,536 octets, giving a total serialized packet size of 7 through 65,542 octets.

Profiles requiring a two-octet CRC trailer have a practical minimum serialized packet size of 8 octets.

Use:

```cpp
std::size_t packetSize = packet.getSerializedSize();
```

for the complete range.

The legacy `getFullPacketLength()` API returns `std::uint16_t` and saturates at `UINT16_MAX` rather than wrapping.

### Packet error control

`PacketErrorControlMode` supports:

- `PacketErrorControlMode::CRC16`, the existing v1 default;
- `PacketErrorControlMode::None`.

In CRC16 mode, CCSDSPack reserves the final two Packet Data Field octets for CRC-16/CCITT-FALSE and includes those octets in Packet Data Length.

The receiver must configure the expected mode before parsing.

### Library architecture

The following diagram shows the main v1.x relationships between the user application, `CCSDS::Manager`, `CCSDS::Packet`, primary-header handling, Packet Data Field handling, secondary-header creation, serialization, validation, CRC16, and utility functions.

![CCSDSPack v1 library architecture](docs/imgs/CCSDSPack_architecture.drawio.png)

## Features

- CCSDS 133.0-B-2 EC2 Space Packet PDU construction and parsing;
- exact bounded parsing with consumed-byte reporting;
- concatenated packet-stream handling;
- optional project-specific CRC16 trailer;
- complete 11-bit APID handling and Idle Packet validation;
- modulo-16384 sequence counting and segmentation utilities;
- one configured Packet Identification value per `CCSDS::Manager` instance;
- custom and opaque secondary-header support;
- exception-free `Result` and `Error` handling;
- Linux and Windows builds;
- optional bare-metal and cross-build targets;
- encoder, decoder, validator, and regression-test executables;
- installed CMake package and shared-library consumer test.

## Documentation

- [Generated API reference](https://exospacelabs.github.io/CCSDSPack/html/)
- [CCSDS 133.0-B-2 EC2 PDU profile](docs/CCSDS_133_0_B_2_PROFILE.md)
- [v1.2 current behavior](docs/V1_2_CURRENT_BEHAVIOUR.md)
- [CLI reference](docs/CLI.md)
- [Configuration reference](docs/CONFIG.md)
- [Error handling](docs/ERROR.md)
- [Packages](docs/PACKAGES.md)
- [Cross-build guide](docs/CROSSBUILD.md)
- [Examples](docs/EXAMPLES.md)

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

Run the regression tester from the configured binary directory:

```bash
./bin/CCSDSPack_tester
```

Install the library and exported CMake package:

```bash
cmake --install build
```

### Build options

| CMake option | Default | Description |
|---|---:|---|
| `CCSDSPACK_BUILD_MCU` | `OFF` | Build the MCU static-library profile |
| `ENABLE_TESTER` | `ON` | Build `CCSDSPack_tester` |
| `ENABLE_ENCODER` | `ON` | Build `ccsds_encoder` |
| `ENABLE_DECODER` | `ON` | Build `ccsds_decoder` |
| `ENABLE_VALIDATOR` | `ON` | Build `ccsds_validator` |

### Windows with MinGW

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

The Windows workflow copies the shared-library DLL beside the test and external-consumer executables before running them.

## CMake integration

After installing CCSDSPack:

```cmake
find_package(CCSDSPack CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ccsdspack::CCSDSPack)
```

For a non-standard install prefix:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/install/prefix
```

## C++ example

```cpp
#include <CCSDSPack.h>

#include <cstdint>
#include <vector>

int main() {
  CCSDS::Packet packetTemplate;

  const auto headerResult = packetTemplate.setPrimaryHeader(CCSDS::PrimaryHeader{
    0,                       // Packet Version Number 000
    0,                       // telemetry packet
    0,                       // no secondary header
    0x123,                   // APID
    CCSDS::UNSEGMENTED,
    0,
    0                        // calculated during serialization
  });

  if (!headerResult) {
    return headerResult.error().code();
  }

  packetTemplate.setDataFieldSize(1024);

  CCSDS::Manager manager;

  const auto templateResult = manager.setPacketTemplate(packetTemplate);
  if (!templateResult) {
    return templateResult.error().code();
  }

  const std::vector<std::uint8_t> inputBytes{0x10, 0x20, 0x30};

  const auto dataResult = manager.setApplicationData(inputBytes);
  if (!dataResult) {
    return dataResult.error().code();
  }

  const auto wire = manager.getPacketsBuffer();
  return wire.empty() ? 1 : 0;
}
```

## Command-line tools

The build can provide:

- `ccsds_encoder`;
- `ccsds_decoder`;
- `ccsds_validator`;
- `CCSDSPack_tester`.

The encoder, decoder, and validator support `crc16` and `none` packet-error-control profiles.

See the [CLI reference](docs/CLI.md) for exact options, concatenated decoding, trailing-byte handling, validation categories, and exit codes.

## Packages and container

Release packages are published under [GitHub Releases](https://github.com/ExoSpaceLabs/CCSDSPack/releases).

Container images are published at:

```bash
docker pull ghcr.io/exospacelabs/ccsdspack:<version>
```

The image contains the library and command-line executables.

Run the installed tester with:

```bash
docker run --rm ghcr.io/exospacelabs/ccsdspack:<version> /usr/bin/CCSDSPack_tester
```

## Legacy secondary headers

The v1 API retains `PusA`, `PusB`, and `PusC` for compatibility with existing projects and configuration files.

Their layouts are project-specific ancillary-data formats:

- `PusA` and `PusB` are fixed-size legacy formats;
- `PusC` contains a variable application-configured byte sequence described as a time-code field;
- none of these classes is claimed to implement an official ECSS PUS revision;
- `PusC` bytes are not automatically validated as a CCSDS time-code format.

Standards-oriented ECSS PUS support remains in the v2.0.0 scope.

## Compatibility with pre-v1.2 packets

Although the public source API remains in the v1 series, v1.2 is not wire-compatible with packets produced using the incorrect length, CRC, sequence, Packet Identification, version, or Idle Packet semantics of earlier releases.

Changes include:

- Packet Data Length;
- CRC coverage;
- parsing boundaries;
- sequence behavior;
- Packet Identification enforcement;
- Packet Version Number validation;
- Idle Packet validation.

Stored or transmitted packets generated by older releases should be regenerated or migrated explicitly before adopting the v1.2 profile.

## License

CCSDSPack is licensed under the Apache License 2.0.

See [LICENSE](LICENSE) and [Notice.md](Notice.md).
