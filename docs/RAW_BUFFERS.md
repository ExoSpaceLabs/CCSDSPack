<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Raw-buffer APIs

[Documentation index](README.md) | [Examples](EXAMPLES.md) | [Error handling](ERROR.md) | [Space Packet profile](CCSDS_133_0_B_2_PROFILE.md)

CCSDSPack v2 keeps `std::vector<std::uint8_t>` as the convenient general-purpose interface and adds pointer-plus-size entry points for UART, DMA, SpaceWire, TCP, shared memory, device-driver, and bare-metal buffers.

The raw API is additive. It does not change packet wire behaviour, packet-level PEC, secondary-header identity, or the vector API.

## Current implementation boundary

In v2.0.0 the public raw signatures still bridge to vector-backed parsing internally. Therefore:

- callers can pass `const std::uint8_t *` plus a byte count directly;
- caller code does not need to construct a vector first;
- v2.0.0 does not claim zero-copy parsing;
- v2.0.0 does not claim globally heap-free Packet/Manager internals;
- later internals can move toward views or fixed storage while preserving the API.

## Declared packet size

`ccsds::buffer::declaredPacketSize()` requires only the six-byte primary header:

```cpp
std::uint8_t primaryHeader[6];
receive(primaryHeader, sizeof(primaryHeader));

const auto size = ccsds::buffer::declaredPacketSize(
  primaryHeader, sizeof(primaryHeader));
if (!size) return size.error().code();

const std::size_t remaining = size.value() - sizeof(primaryHeader);
```

The returned size is `6 + Packet Data Length + 1`. The function checks the pointer, the six-byte minimum, and Packet Version Number `000`; it does not require the body or attempt CRC/PUS validation.

## Generic raw parsing

```cpp
ccsds::Packet packet;
packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet, rxBuffer, receivedBytes);
if (!consumed) return consumed.error().code();
```

The Packet error-control mode remains generic packet policy and must match the expected wire stream.

## Typed PUS raw parsing

PUS revision and direction come from the concrete header type. Default tailoring requires no prior setup:

```cpp
ccsds::Packet packet;
const auto consumed =
  ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TcHeader>(
    packet, rxBuffer, receivedBytes);
if (!consumed) return consumed.error().code();
```

When the wire uses optional tailoring, pass the same tailoring constructor argument used for vector parsing:

```cpp
ccsds::pus::rev_c::TmTailoring tailoring;
tailoring.timestampPresent = true;
tailoring.cuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Implicit,
  4,
  0
};

const auto consumed =
  ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TmHeader>(
    packet, rxBuffer, receivedBytes, tailoring);
```

The parser verifies that the CCSDS primary-header Packet Type matches the intrinsic `TcHeader`/`TmHeader` direction. There is no separate MissionProfile or direction enum to synchronize.

The runtime string-selector overload remains available for dynamic/configuration-driven use. For variable-length custom headers, supply the explicit header size; opaque secondary headers can use the `headerDataSizeBytes` overload.

## Manager raw-buffer overloads

The complete Packet template remains the Manager stream contract, including packet PEC, secondary-header type, and PUS tailoring.

```cpp
manager.setApplicationData(payload, payloadSize);
manager.addPacketFromBuffer(packetBytes, packetSize);
manager.load(streamBytes, streamSize);
```

These overloads retain the same Packet Identification, secondary-header contract, sequence, CRC, transactional-load, and optional synchronization-pattern behaviour as the vector paths.

## Read-only Manager views

```cpp
const ccsds::Manager &view = manager;
const ccsds::Packet &packetTemplate = view.getTemplateReference();
const std::vector<ccsds::Packet> &packets = view.getPacketsReference();
const ccsds::Validator &validator = view.getValidatorReference();
```

The references remain owned by Manager and must not outlive it.

## Symbolic errors

```cpp
const auto result = ccsds::buffer::deserializeBounded(packet, rx, size);
if (!result) {
  log(ccsds::errorCodeName(result.error().code()),
      result.error().message());
}
```

`errorCodeName()` complements `validationCodeName()` and performs no allocation.

## Bare-metal use

The raw-buffer header is installed and available under `CCSDS_MCU`. It remains C++17-compatible and adds no RTTI or exception requirement.

The Cortex-M compile/link probe exercises:

- `ccsds::buffer::declaredPacketSize()`;
- generic raw bounded Packet parsing;
- typed raw PUS-C TC parsing;
- the structured Validator.

This proves public-header/link compatibility under the MCU profile. Physical target execution remains a separate release-validation gate.

## Standalone installed-package examples

The example suite includes:

- `raw_buffer_packet`: primary-header size inspection and raw Packet parsing;
- `raw_buffer_manager`: raw Manager ingestion and const-reference inspection;
- PUS-C examples demonstrating default and tailored concrete headers.

Build against an installed package:

```bash
./example/build_examples.sh raw_buffer_packet /path/to/install/prefix
./example/build_examples.sh raw_buffer_manager /path/to/install/prefix
```

The complete installed-package example suite is built and executed by Linux and Windows CI.
