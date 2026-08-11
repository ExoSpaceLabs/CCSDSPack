<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Examples

[Documentation index](README.md) | [Raw buffers](RAW_BUFFERS.md) | [Configuration](CONFIG.md) | [Structured validation](VALIDATION.md) | [Space Packet profile](CCSDS_133_0_B_2_PROFILE.md)

These examples use the v2 C++17 API. They create CCSDS Space Packets with Packet Version Number `000`. The default packet-error-control profile is the project-specific CRC16 trailer; configure `PacketErrorControlMode::None` explicitly for CRC-free packets.

## Build integration

After installing CCSDSPack:

```cmake
find_package(CCSDSPack 2.0 CONFIG REQUIRED)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE ccsdspack::CCSDSPack)
target_compile_features(example PRIVATE cxx_std_17)
```

Six complete installed-package consumers are maintained under [`example/`](../example/README.md). Each has its own `CMakeLists.txt` and uses `find_package(CCSDSPack 2.0 CONFIG REQUIRED)`.

After installing the library, build every example or one target directory:

```bash
./example/build_examples.sh all /path/to/install/prefix
./example/build_examples.sh raw_buffer_packet /path/to/install/prefix
./example/build_examples.sh raw_buffer_manager /path/to/install/prefix
./example/build_examples.sh custom_secondary_header /path/to/install/prefix
```

The complete example set is built and executed by the Linux and Windows installed-package CI paths.

## Create and parse one packet with vectors

```cpp
#include <CCSDSPack.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  ccsds::Packet packet;
  packet.setDataFieldSize(1024);

  const auto headerResult = packet.setPrimaryHeader(ccsds::PrimaryHeader{
    0,                       // Packet Version Number: must be 000
    0,                       // Packet Type: telemetry
    0,                       // no secondary header
    0x123,                   // APID
    ccsds::UNSEGMENTED,
    0,                       // Packet Sequence Count
    0                        // calculated by serialize()
  });
  if (!headerResult) return headerResult.error().code();

  const auto dataResult = packet.setApplicationData({0x10, 0x20, 0x30});
  if (!dataResult) return dataResult.error().code();

  const auto wireResult = packet.serialize();
  if (!wireResult) return wireResult.error().code();
  const auto &wire = wireResult.value();

  ccsds::Packet decoded; // CRC16 is also the receiving default
  const auto consumedResult = decoded.deserializeBounded(wire);
  if (!consumedResult) return consumedResult.error().code();

  std::cout << "Consumed " << consumedResult.value()
            << " bytes, APID=" << decoded.getPrimaryHeader().getAPID()
            << ", application bytes=" << decoded.getApplicationDataBytes().size()
            << '\n';
  return 0;
}
```

`serialize()` returns `ResultBuffer` and reports the exact finalization error. `update()` returns `ResultBool` when finalization without bytes is required. Getters inspect stored state and do not perform hidden finalization.

The vector APIs remain the most convenient interface when the application already owns data in vectors.

## Receive from a raw transport buffer

Use `ccsds::buffer::declaredPacketSize()` after receiving the six-byte primary header. The packet body does not need to be present yet:

```cpp
std::uint8_t header[6];
receive(header, sizeof(header));

const auto declared = ccsds::buffer::declaredPacketSize(header, sizeof(header));
if (!declared) return declared.error().code();

const std::size_t remaining = declared.value() - sizeof(header);
```

Once the complete packet is available in transport-owned storage:

```cpp
ccsds::Packet packet;
packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet, rxBuffer, receivedBytes);
if (!consumed) {
  log(ccsds::errorCodeName(consumed.error().code()),
      consumed.error().message());
  return consumed.error().code();
}
```

The public pointer-plus-size API is intended to remain stable as internals evolve. In v2.0.0 the raw parsing adapters still bridge through vector-backed parsing internally, so this is not yet a zero-copy claim. See [RAW_BUFFERS.md](RAW_BUFFERS.md).

## Use raw buffers with Manager

A Manager can accept fixed or externally owned application and stream buffers directly:

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
const auto setResult = sender.setApplicationData(payload, payloadSize);
if (!setResult) return setResult.error().code();

const auto wire = sender.getPacketsBuffer();
if (!wire) return wire.error().code();

ccsds::Manager receiver;
receiver.setPacketTemplate(packetTemplate);
const auto loadResult = receiver.load(wire.value().data(), wire.value().size());
if (!loadResult) return loadResult.error().code();
```

For read-only inspection without copying the packet collection:

```cpp
const ccsds::Manager &view = receiver;
const auto &packetTemplateRef = view.getTemplateReference();
const auto &packets = view.getPacketsReference();
const auto &validator = view.getValidatorReference();
```

These references remain owned by the Manager.

## Parse PUS from a raw buffer

PUS raw parsing retains the same explicit mission-profile and selector contract:

```cpp
auto profile = ccsds::pus::makeProfile(
  ccsds::pus::Revision::C,
  ccsds::pus::Direction::Telecommand);

ccsds::Packet packet;
const auto profileResult = packet.setMissionProfile(profile);
if (!profileResult) return profileResult.error().code();

const auto consumed = ccsds::buffer::deserializeBounded(
  packet,
  rxBuffer,
  receivedBytes,
  ccsds::pus::selector(profile.pusRevision, profile.direction));
if (!consumed) return consumed.error().code();
```

The raw adapter does not infer a PUS revision or direction from bytes.

## Validate a packet with named checks

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);

if (!report.valid()) {
  for (const auto &check : report) {
    if (!check.passed) {
      handleValidationFailure(check.code,
                              ccsds::validationCodeName(check.code));
    }
  }
}
```

Query a specific condition without relying on report indices:

```cpp
if (report.failed(ccsds::ValidationCode::PacketDataLength)) {
  handleLengthFailure();
}
```

The same Validator API is available in the C++17 `CCSDS_MCU` build. The report uses fixed `std::array` storage and performs no dynamic allocation itself. See [VALIDATION.md](VALIDATION.md).

## Segment a payload with Manager

One Manager represents one complete Packet Identification value and one Packet Sequence Count stream.

```cpp
ccsds::Packet packetTemplate;
packetTemplate.setDataFieldSize(1024);

const auto headerResult = packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0
});
if (!headerResult) return headerResult.error().code();

ccsds::Manager manager;
const auto templateResult = manager.setPacketTemplate(packetTemplate);
if (!templateResult) return templateResult.error().code();

manager.setDataFieldSize(1024);
std::vector<std::uint8_t> payload(5000, 0xAB);
const auto dataResult = manager.setApplicationData(payload);
if (!dataResult) return dataResult.error().code();

const auto streamResult = manager.getPacketsBuffer();
if (!streamResult) return streamResult.error().code();
```

A one-packet result uses `UNSEGMENTED`. A multi-packet result uses `FIRST_SEGMENT`, zero or more `CONTINUING_SEGMENT` packets, and `LAST_SEGMENT`. Automatic sequence counting advances once per packet and wraps modulo 16384.

## Parse concatenated packets manually

Vector-owned stream iteration remains valid:

```cpp
std::size_t offset = 0;
while (offset < stream.size()) {
  std::vector<std::uint8_t> remaining(stream.begin() + offset, stream.end());
  ccsds::Packet packet;
  const auto consumed = packet.deserializeBounded(remaining);
  if (!consumed) return consumed.error();
  offset += consumed.value();
}
```

For externally owned contiguous memory, avoid constructing the `remaining` vector in caller code and use the raw adapter instead:

```cpp
std::size_t offset = 0;
while (offset < streamSize) {
  ccsds::Packet packet;
  const auto consumed = ccsds::buffer::deserializeBounded(
    packet, streamData + offset, streamSize - offset);
  if (!consumed) return consumed.error();
  offset += consumed.value();
}
```

The current adapter may still allocate internally; the caller-facing API no longer requires ownership transfer into a vector.

## Create CRC-free packets

The sender and receiver must both select `None`. The mode is never inferred from packet bytes.

```cpp
ccsds::Packet sender;
sender.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
sender.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0
});
sender.setApplicationData({0x01, 0x02});
const auto wireResult = sender.serialize();
if (!wireResult) return wireResult.error().code();

ccsds::Packet receiver;
receiver.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
const auto consumed = receiver.deserializeBounded(wireResult.value());
if (!consumed) return consumed.error().code();
```

## Add an opaque secondary header

For mission-specific opaque bytes that do not use a registered typed header, use the `BufferHeader` path through `setSecondaryHeader()`:

```cpp
ccsds::Packet packet;
packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 1, 0x123, ccsds::UNSEGMENTED, 0, 0
});
packet.setSecondaryHeader(std::vector<std::uint8_t>{0xAA, 0x55});
packet.setApplicationData({0x10, 0x20});
const auto wireResult = packet.serialize();
if (!wireResult) return wireResult.error().code();
```

When parsing an opaque secondary header, provide its byte count. Both vector and raw adapters support this contract.

## Use a configuration file

A minimal template configuration is:

```ini
mission_profile:string=generic
ccsds_packet_error_control:string=crc16

ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_secondary_header_flag:bool=false
ccsds_APID:int=0x123
ccsds_segmented:bool=false

data_field_size:int=1024

define_secondary_header:bool=false
```

Load it directly into a Manager:

```cpp
ccsds::Manager manager;
const auto templateResult = manager.loadTemplateConfigFile("template.cfg");
if (!templateResult) return templateResult.error().code();
```

See [CONFIG.md](CONFIG.md) for all keys, PUS/CUC profiles, and Idle Packet constraints. Complete CLI-ready configurations are in [`example/config`](../example/config).

## Command-line equivalent

```bash
ccsds_encoder -i payload.bin -o packets.bin -c template.cfg
ccsds_validator -i packets.bin -c template.cfg --verbose
ccsds_decoder -i packets.bin -o recovered.bin -c template.cfg
```

Use `--packet-error-control none` consistently on all three tools for a CRC-free stream. See [CLI.md](CLI.md) for complete options and trailing-byte handling.
