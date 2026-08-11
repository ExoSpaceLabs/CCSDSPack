<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Examples

[Documentation index](README.md) | [Raw buffers](RAW_BUFFERS.md) | [Configuration](CONFIG.md) | [Structured validation](VALIDATION.md) | [Space Packet profile](CCSDS_133_0_B_2_PROFILE.md)

These examples use the v2 C++17 API. Packet-level CRC16 is the default; use `PacketErrorControlMode::None` explicitly for CRC-free streams. PUS revision/direction is carried by the concrete PUS header type, while optional mission layout choices use direction-specific tailoring structs.

## Build integration

```cmake
find_package(CCSDSPack 2.0 CONFIG REQUIRED)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE ccsdspack::CCSDSPack)
target_compile_features(example PRIVATE cxx_std_17)
```

Six installed-package consumers are maintained under [`example/`](../example/README.md). Build all or one target:

```bash
./example/build_examples.sh all /path/to/install/prefix
./example/build_examples.sh raw_buffer_packet /path/to/install/prefix
./example/build_examples.sh raw_buffer_manager /path/to/install/prefix
./example/build_examples.sh custom_secondary_header /path/to/install/prefix
./example/build_examples.sh pus_c_telecommand /path/to/install/prefix
./example/build_examples.sh pus_c_telemetry /path/to/install/prefix
```

Linux and Windows CI compile and execute the complete installed-package example set.

## Generic packet with vectors

```cpp
#include <CCSDSPack.h>

ccsds::Packet packet;
packet.setDataFieldSize(1024);

const auto header = packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0
});
if (!header) return header.error().code();

const auto data = packet.setApplicationData({0x10, 0x20, 0x30});
if (!data) return data.error().code();

const auto wire = packet.serialize();
if (!wire) return wire.error().code();

ccsds::Packet decoded;
const auto consumed = decoded.deserializeBounded(wire.value());
if (!consumed) return consumed.error().code();
```

`serialize()` returns `ResultBuffer`; `update()` returns `ResultBool`. Getters inspect stored state and do not perform hidden finalization.

## PUS-C telecommand with defaults

No profile object is required:

```cpp
ccsds::Packet packet;
packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0
});

const auto secondary = packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TcHeader>(
    17,       // service type
    1,        // service subtype
    0x1234,   // source ID
    0x09));   // acknowledgement flags
if (!secondary) return secondary.error().code();
```

`rev_c::TcHeader` intrinsically means PUS-C Telecommand. Installing it sets the CCSDS secondary-header flag and synchronizes Packet Type to Telecommand.

The PUS-C TC source-ID width is fixed at two octets by the supported layout.

## PUS-C telemetry with optional CUC tailoring

```cpp
ccsds::pus::rev_c::TmTailoring tailoring;
tailoring.timestampPresent = true;
tailoring.cuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Implicit,
  4,
  0
};

ccsds::Packet packet;
packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>(
    tailoring,
    3, 25, 7, 0x1234, 2,
    ccsds::time::CucTime{0x01020304, 0}));
```

Default PUS-C TM has no timestamp. Tailoring is supplied only when the wire layout requires an optional choice such as CUC time or spare octets.

## PUS deserialization

Preinstall a parsing schema:

```cpp
ccsds::Packet decoded;
decoded.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>());

const auto parsed = decoded.deserialize(wire);
```

Or use the typed convenience API:

```cpp
ccsds::Packet decoded;
const auto parsed =
  decoded.deserialize<ccsds::pus::rev_c::TmHeader>(wire);
```

Tailored wire layouts pass the same tailoring object to the typed parser:

```cpp
const auto parsed =
  decoded.deserialize<ccsds::pus::rev_c::TmHeader>(
    wire, tailoring);
```

The parser rejects a CCSDS Packet Type that contradicts the concrete `TcHeader`/`TmHeader` direction.

## Receive from a raw transport buffer

After receiving the six-byte primary header:

```cpp
std::uint8_t header[6];
receive(header, sizeof(header));

const auto declared = ccsds::buffer::declaredPacketSize(header, sizeof(header));
if (!declared) return declared.error().code();
```

For a complete generic packet:

```cpp
ccsds::Packet packet;
packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet, rxBuffer, receivedBytes);
```

For a PUS-C TC packet with default tailoring:

```cpp
const auto consumed =
  ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TcHeader>(
    packet, rxBuffer, receivedBytes);
```

For tailored PUS-C TM:

```cpp
const auto consumed =
  ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TmHeader>(
    packet, rxBuffer, receivedBytes, tailoring);
```

The raw API currently bridges to vector-backed internals. It establishes a stable transport boundary for later zero-copy/heap-free work but does not claim that the v2.0 implementation is allocation-free.

See [RAW_BUFFERS.md](RAW_BUFFERS.md).

## Manager with a complete Packet template

Manager has no independent profile state. Its Packet template carries Packet Identification, PEC, secondary-header type, and any PUS tailoring.

```cpp
ccsds::Packet packetTemplate;
packetTemplate.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x155, ccsds::UNSEGMENTED, 0, 0
});
packetTemplate.setDataFieldSize(256);

ccsds::Manager sender;
sender.setPacketTemplate(packetTemplate);

std::uint8_t payload[128];
const std::size_t payloadSize = acquirePayload(payload, sizeof(payload));
sender.setApplicationData(payload, payloadSize);

const auto wire = sender.getPacketsBuffer();
if (!wire) return wire.error().code();

ccsds::Manager receiver;
receiver.setPacketTemplate(packetTemplate);
const auto loaded = receiver.load(wire.value().data(), wire.value().size());
if (!loaded) return loaded.error().code();
```

A PUS Manager works the same way: install the desired concrete PUS header/tailoring in `packetTemplate` before passing it to Manager.

For read-only inspection without copies:

```cpp
const ccsds::Manager &view = receiver;
const auto &packetTemplateRef = view.getTemplateReference();
const auto &packets = view.getPacketsReference();
const auto &validator = view.getValidatorReference();
```

## Validate with named checks

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);

if (report.failed(ccsds::ValidationCode::PacketDataLength)) {
  handleLengthFailure();
}

if (report.failed(ccsds::ValidationCode::PusTailoring)) {
  handlePusTailoringFailure();
}
```

The same fixed-capacity Validator API is available under `CCSDS_MCU`.

## Segment application data with Manager

One Manager represents one Packet Identification value and one Packet Sequence Count stream.

```cpp
ccsds::Packet packetTemplate;
packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0
});
packetTemplate.setDataFieldSize(1024);

ccsds::Manager manager;
manager.setPacketTemplate(packetTemplate);

std::vector<std::uint8_t> payload(5000, 0xAB);
const auto result = manager.setApplicationData(payload);
```

A one-packet result uses `UNSEGMENTED`. A multi-packet result uses `FIRST_SEGMENT`, zero or more `CONTINUING_SEGMENT` packets, and `LAST_SEGMENT`. Automatic sequence counting wraps modulo 16384.

## CRC-free packets

Sender and receiver both select `None`; the mode is not inferred from packet bytes.

```cpp
ccsds::Packet sender;
sender.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

ccsds::Packet receiver;
receiver.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
```

This is generic packet policy and is independent of whether a PUS/custom secondary header is installed.

## Opaque secondary headers

```cpp
ccsds::Packet packet;
packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0
});
packet.setSecondaryHeader(std::vector<std::uint8_t>{0xAA, 0x55});
packet.setApplicationData({0x10, 0x20});
```

When parsing an opaque header, provide its byte count. Registered custom headers may override `SecondaryHeaderAbstract::getDirection()` when their format has an intrinsic direction; otherwise they remain direction-neutral.

## Configuration file

Minimal generic configuration:

```ini
ccsds_packet_error_control:string=crc16
ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_APID:int=0x123
ccsds_segmented:bool=false
data_field_size:int=1024
define_secondary_header:bool=false
```

PUS identity is selected by the concrete selector, not separate revision/direction/profile keys:

```ini
define_secondary_header:bool=true
secondary_header_type:string=PUS:revC:TC
pus_service_type:uint=17
pus_service_subtype:uint=1
pus_acknowledgement_flags:uint=9
pus_source_id:uint=0x1234
```

See [CONFIG.md](CONFIG.md) and [`example/config`](../example/config).

## Command-line equivalent

```bash
ccsds_encoder -i payload.bin -o packets.bin -c template.cfg
ccsds_validator -i packets.bin -c template.cfg --verbose
ccsds_decoder -i packets.bin -o recovered.bin -c template.cfg
```

Use `--packet-error-control none` consistently for CRC-free streams. See [CLI.md](CLI.md).
