# Examples

[Documentation index](README.md) | [Raw buffers](RAW_BUFFERS.md) | [Configuration](CONFIG.md) | [Validation](VALIDATION.md)

The examples use the C++17 v2 API and the same ownership model as the library: Packet owns generic packet policy, concrete secondary headers own their layout, and Manager uses a complete Packet template.

## Installed-package integration

```cmake
find_package(CCSDSPack 2.0 CONFIG REQUIRED)
add_executable(example main.cpp)
target_link_libraries(example PRIVATE ccsdspack::CCSDSPack)
target_compile_features(example PRIVATE cxx_std_17)
```

The `example/` directory contains installed-package consumers for generic packets, custom secondary headers, PUS-C TC/TM, raw Packet parsing, and raw Manager ingestion. Linux and Windows CI build and execute the complete set.

## Generic Packet

```cpp
ccsds::Packet packet;
packet.setDataFieldSize(1024);
packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});
packet.setApplicationData({0x10, 0x20, 0x30});

const auto wire = packet.serialize();
if (!wire) return wire.error().code();
```

`serialize()` and `update()` return checked Result types. Inspection getters do not perform hidden finalization.

## PUS-C telecommand

```cpp
ccsds::Packet packet;
packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});

packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TcHeader>(
    17, 1, 0x1234, 0x09));
```

The concrete header identifies PUS-C Telecommand and synchronizes Packet Type. Its source ID is two octets.

## PUS-C telemetry with CUC

```cpp
ccsds::pus::rev_c::TmTailoring tailoring;
tailoring.timestampPresent = true;
tailoring.cuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Implicit,
  4, 0
};

ccsds::Packet packet;
packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>(
    tailoring, 3, 25, 7, 0x1234, 2,
    ccsds::time::CucTime{0x01020304, 0}));
```

## Typed PUS parsing

```cpp
ccsds::Packet decoded;
const auto parsed =
  decoded.deserialize<ccsds::pus::rev_c::TmHeader>(wire);
```

A preinstalled concrete header or tailoring constructor argument can be used when the wire layout requires it.

## Raw transport parsing

```cpp
const auto declared =
  ccsds::buffer::declaredPacketSize(primaryHeader, 6U);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet, rxBuffer, receivedBytes);
```

Typed PUS raw parsing mirrors `Packet::deserialize<HeaderT>()`.

## Manager

```cpp
ccsds::Packet packetTemplate;
packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x155, ccsds::UNSEGMENTED, 0, 0});
packetTemplate.setDataFieldSize(256);

ccsds::Manager sender;
sender.setPacketTemplate(packetTemplate);
sender.setApplicationData(payload, payloadSize);

const auto wire = sender.getPacketsBuffer();
if (!wire) return wire.error().code();
```

A PUS Manager uses the same pattern with the desired concrete PUS header installed in `packetTemplate`.

## Structured validation

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);

if (report.failed(ccsds::ValidationCode::PusTailoring)) {
  handlePusTailoringFailure();
}
```

## CRC-free packets

```cpp
packet.setPacketErrorControlMode(
  ccsds::PacketErrorControlMode::None);
```

Sender and receiver select the same Packet-level mode; it is independent of the secondary-header type.

## Configuration

```ini
ccsds_packet_error_control:string=crc16
ccsds_version_number:int=0
ccsds_APID:int=0x123
ccsds_segmented:bool=false
define_secondary_header:bool=true
secondary_header_type:string=PUS:revC:TC
pus_service_type:uint=17
pus_service_subtype:uint=1
pus_acknowledgement_flags:uint=9
pus_source_id:uint=0x1234
```

See [`example/config`](../example/config) for complete configurations.
