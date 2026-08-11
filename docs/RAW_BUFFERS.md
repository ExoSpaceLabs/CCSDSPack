<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Raw-buffer APIs

[Documentation index](README.md) | [Examples](EXAMPLES.md) | [Error handling](ERROR.md)

CCSDSPack supports both `std::vector<std::uint8_t>` interfaces and pointer-plus-size interfaces for UART, DMA, SpaceWire, TCP, shared memory, device-driver, and bare-metal buffers.

## Implementation boundary

The v2.0.0 raw entry points currently bridge through vector-backed parsing internally. Callers can therefore pass transport-owned memory directly, while the implementation makes no zero-copy or globally heap-free claim.

## Declared packet size

`ccsds::buffer::declaredPacketSize()` requires only the six-byte primary header:

```cpp
const auto size = ccsds::buffer::declaredPacketSize(
  primaryHeader, sizeof(primaryHeader));
if (!size) return size.error().code();
```

The result is `6 + Packet Data Length + 1`. The function validates the pointer, six-byte minimum, and Packet Version Number; it does not parse the body or validate CRC/PUS state.

## Generic raw parsing

```cpp
ccsds::Packet packet;
packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet, rxBuffer, receivedBytes);
```

The Packet error-control mode must match the expected wire stream.

## Typed PUS raw parsing

```cpp
ccsds::Packet packet;
const auto consumed =
  ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TcHeader>(
    packet, rxBuffer, receivedBytes);
```

Optional tailoring can be passed as a constructor argument:

```cpp
ccsds::pus::rev_c::TmTailoring tailoring;
tailoring.timestampPresent = true;
tailoring.cuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Implicit,
  4, 0
};

const auto consumed =
  ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TmHeader>(
    packet, rxBuffer, receivedBytes, tailoring);
```

The parser validates the concrete header direction against the CCSDS Packet Type. Runtime string selectors remain available for dynamic/configuration-driven parsing. Variable-length custom and opaque headers can supply an explicit header byte count.

## Manager overloads

The Manager Packet template is the stream contract, including Packet Identification, packet-level PEC, secondary-header type, and PUS tailoring.

```cpp
manager.setApplicationData(payload, payloadSize);
manager.addPacketFromBuffer(packetBytes, packetSize);
manager.load(streamBytes, streamSize);
```

These overloads preserve the same identifier, sequence, CRC, secondary-header, transactional-load, and synchronization-pattern behavior as vector paths.

## Const Manager views

```cpp
const ccsds::Manager &view = manager;
const ccsds::Packet &packetTemplate = view.getTemplateReference();
const auto &packets = view.getPacketsReference();
const ccsds::Validator &validator = view.getValidatorReference();
```

References remain owned by Manager and must not outlive it.

## Bare-metal use

Raw-buffer APIs are available under `CCSDS_MCU` and add no RTTI or exception requirement. The Cortex-M compile/link probe exercises declared-size inspection, generic raw bounded parsing, typed raw PUS parsing, and structured validation. Physical execution remains a separate release-validation gate.

## Examples

The installed-package example suite includes `raw_buffer_packet`, `raw_buffer_manager`, and PUS-C examples. Linux and Windows CI build and execute the example set against the installed package.
