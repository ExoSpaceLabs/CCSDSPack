<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Raw-buffer APIs

[Documentation index](README.md) | [Examples](EXAMPLES.md) | [Error handling](ERROR.md) | [Space Packet profile](CCSDS_133_0_B_2_PROFILE.md)

CCSDSPack v2 keeps its `std::vector<std::uint8_t>` APIs as the convenient general-purpose interface and adds pointer-plus-size entry points for transport-owned buffers. These APIs are intended for UART, DMA, SpaceWire, TCP, shared-memory, device-driver, and bare-metal integration where received bytes already exist in a fixed or externally owned buffer.

The raw-buffer API is **additive**. It does not change packet wire behaviour, PUS profiles, or the existing vector API.

## Current implementation boundary

The public raw-buffer signatures are designed so callers do not have to change when parser internals become more memory-efficient later. In v2.0.0, raw Packet parsing and raw Manager ingestion currently bridge to the existing vector-backed implementation internally.

Therefore:

- callers may pass `const std::uint8_t *` plus a byte count directly;
- callers do not need to construct a vector themselves;
- v2.0.0 does **not** claim zero-copy parsing;
- v2.0.0 does **not** claim a globally heap-free Packet/Manager implementation;
- a later release may replace the internal copy with view/span-like or fixed-storage parsing while preserving these public signatures.

This distinction is intentional. A stable transport-facing API is useful now; pretending the current implementation is already allocation-free would be less useful and considerably more creative.

## Determine the declared packet size

`ccsds::buffer::declaredPacketSize()` requires only the six-byte CCSDS Space Packet primary header:

```cpp
std::uint8_t primaryHeader[6];
receive(primaryHeader, sizeof(primaryHeader));

const auto size = ccsds::buffer::declaredPacketSize(
  primaryHeader, sizeof(primaryHeader));
if (!size) {
  handleError(size.error());
  return;
}

const std::size_t remaining = size.value() - sizeof(primaryHeader);
```

The returned value is:

```text
6 + Packet Data Length + 1
```

The function validates that:

- the pointer is not null;
- at least six readable bytes are available;
- Packet Version Number is `000`.

It does not require the packet body to be present and does not attempt CRC, PUS, or application-data validation. Its purpose is to tell a transport how many bytes belong to the complete packet.

The maximum returned Space Packet size is 65,542 bytes.

A vector convenience overload also exists:

```cpp
const auto size = ccsds::buffer::declaredPacketSize(bytes);
```

## Parse a generic raw packet

For a complete generic packet buffer:

```cpp
ccsds::Packet packet;
packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet, rxBuffer, receivedBytes);
if (!consumed) {
  handleError(consumed.error());
  return;
}
```

`deserializeBounded()` returns the declared number of consumed bytes and leaves any following transport bytes conceptually outside the packet boundary, matching the existing vector API semantics.

The non-bounded adapter is also available:

```cpp
const auto result = ccsds::buffer::deserialize(
  packet, rxBuffer, receivedBytes);
```

## Parse PUS or registered custom headers

The typed raw-buffer overload accepts the same selector used by the vector API:

```cpp
auto profile = ccsds::pus::makeProfile(
  ccsds::pus::Revision::C,
  ccsds::pus::Direction::Telecommand);

ccsds::Packet packet;
packet.setMissionProfile(profile);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet,
  rxBuffer,
  receivedBytes,
  ccsds::pus::selector(profile.pusRevision, profile.direction));
```

PUS parsing still requires the explicit matching mission profile and canonical PUS selector. The raw adapter does not weaken revision, direction, Packet Type, packet-error-control, or secondary-header validation.

For a variable-length custom secondary header, supply its explicit size using the existing typed overload argument. An opaque secondary header can use the raw overload that accepts `headerDataSizeBytes`.

## Manager raw-buffer overloads

`ccsds::Manager` exposes direct pointer-plus-size overloads for the three common embedded boundaries.

### Application data

```cpp
std::uint8_t payload[256];
const std::size_t payloadSize = receivePayload(payload, sizeof(payload));

const auto result = manager.setApplicationData(payload, payloadSize);
```

### One packet

```cpp
const auto result = manager.addPacketFromBuffer(packetBytes, packetSize);
```

### Concatenated packet stream

```cpp
const auto result = manager.load(streamBytes, streamSize);
```

These overloads retain the same identifier binding, mission-profile, sequence, CRC, transactional-load, and optional synchronization-pattern behaviour as their vector counterparts.

## Read-only Manager views

Copy-returning getters remain available for convenience. For code that only needs to inspect existing state, const Manager objects now expose zero-copy references:

```cpp
const ccsds::Manager &view = manager;

const ccsds::Packet &packetTemplate = view.getTemplateReference();
const std::vector<ccsds::Packet> &packets = view.getPacketsReference();
const ccsds::Validator &validator = view.getValidatorReference();
```

These accessors do not copy the template or packet collection. The returned references remain owned by the Manager and must not outlive it.

The existing mutable reference APIs are retained where they already existed. Direct mutable access can bypass Manager bookkeeping, so const views are preferred when modification is unnecessary.

## Symbolic errors

`ccsds::errorCodeName()` provides a stable symbolic label without allocation:

```cpp
const auto result = ccsds::buffer::deserializeBounded(packet, rx, size);
if (!result) {
  log(ccsds::errorCodeName(result.error().code()),
      result.error().message());
}
```

This complements `ccsds::validationCodeName()` used by the structured Validator.

## Bare-metal use

The raw-buffer header is part of the installed public API and is available when `CCSDS_MCU` is defined. The APIs remain C++17-compatible and introduce no RTTI or exception requirement.

The Cortex-M compile/link probe exercises:

- `ccsds::buffer::declaredPacketSize()`;
- raw bounded Packet parsing;
- the existing structured Validator and representative PUS-C path.

This proves public-header and link compatibility under the MCU build profile. Actual target hardware execution remains a separate v2 release-validation gate.

## Standalone installed-package examples

Two examples are maintained under [`example/`](../example/README.md):

- `raw_buffer_packet`: primary-header size inspection and raw Packet parsing;
- `raw_buffer_manager`: raw Manager application/stream ingestion and const-reference inspection.

Build them against an installed CCSDSPack package:

```bash
./example/build_examples.sh raw_buffer_packet /path/to/install/prefix
./example/build_examples.sh raw_buffer_manager /path/to/install/prefix
```

The complete example suite is built and executed by the Linux and Windows installed-package CI paths.
