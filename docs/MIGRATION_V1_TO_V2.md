# Migrating CCSDSPack v1.2 to v2.0

This document is the authoritative source for source, configuration, CLI, package, and wire-format changes when upgrading a v1.2 application to v2.0. Current v2 behavior is documented elsewhere without migration history.

## Migration summary

The main upgrade areas are:

- public namespace `CCSDS` to `ccsds`;
- secondary-header terminology and APIs;
- standards-oriented PUS-A/PUS-C TC/TM concrete types;
- Packet-centered ownership instead of duplicated profile state;
- checked Result-based finalization/serialization;
- named structured Validator reports;
- updated hosted configuration selectors;
- raw pointer-plus-size transport interfaces;
- package major version/SOVERSION 2.

## Namespace

```cpp
// v1.2
CCSDS::Packet packet;

// v2.0
ccsds::Packet packet;
```

There is no uppercase namespace compatibility alias. The installed CMake package remains `CCSDSPack` and the imported target remains `ccsdspack::CCSDSPack`.

## Generic Packet example

Typical v1.2 source:

```cpp
CCSDS::Packet packet;
packet.setPrimaryHeader(CCSDS::PrimaryHeader{
  0, 0, 0, 0x123, CCSDS::UNSEGMENTED, 0, 0});
packet.setApplicationData({0x10, 0x20});
const auto wire = packet.serialize();
```

v2.0 source:

```cpp
ccsds::Packet packet;
packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});
packet.setApplicationData({0x10, 0x20});

const auto wire = packet.serialize();
if (!wire) return wire.error().code();
send(wire.value());
```

## PUS migration

The v1.2 `PusA`, `PusB`, and `PusC` classes were project-specific formats and do not have a byte-for-byte standards mapping. Migration therefore starts from the intended standard revision/direction and mission fields, not from a class-name substitution.

| v1.2 concept | v2.0 direction |
|---|---|
| `PusA` project header | select the intended `rev_a::TcHeader` or `rev_a::TmHeader` |
| `PusC` project header | select the intended `rev_c::TcHeader` or `rev_c::TmHeader` |
| `PusB` event field | place the event identifier in the appropriate service/application data or a deliberately proprietary custom secondary header |
| mixed secondary-header factory | custom `ccsds::SecondaryHeaderFactory` plus fixed `ccsds::pus::SecondaryHeaderFactory` |

### PUS-A telecommand

Representative v1.2 project header:

```cpp
PusA oldHeader(1, serviceType, serviceSubtype, sourceId, dataLength);
```

v2.0 PUS-A TC:

```cpp
ccsds::pus::rev_a::TcTailoring tailoring;
tailoring.sourceIdOctets = 1;

packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_a::TcHeader>(
    tailoring, serviceType, serviceSubtype, sourceId, acknowledgementFlags));
```

### PUS-A telemetry

The same v1.2 project header must be reinterpreted from mission intent rather than wire compatibility. A v2.0 PUS-A TM is explicit:

```cpp
ccsds::pus::rev_a::TmTailoring tailoring;
tailoring.destinationIdOctets = 1;
tailoring.packetSubcounterPresent = true;

packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_a::TmHeader>(
    tailoring, serviceType, serviceSubtype,
    packetSubcounter, destinationId, timestamp));
```

### PUS-C telecommand

Representative v1.2 variable-time project header:

```cpp
PusC oldHeader(2, serviceType, serviceSubtype,
               sourceId, timeCodeBytes, dataLength);
```

v2.0 PUS-C TC:

```cpp
packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TcHeader>(
    serviceType, serviceSubtype, sourceId, acknowledgementFlags));
```

PUS-C TC source ID is two octets in the supported layout.

### PUS-C telemetry

```cpp
ccsds::pus::rev_c::TmTailoring tailoring;
tailoring.timestampPresent = true;
tailoring.cuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Explicit,
  4, 2
};

packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>(
    tailoring, serviceType, serviceSubtype,
    messageTypeCounter, destinationId,
    timeReferenceStatus, timestamp));
```

PUS-C TM destination ID is two octets.

## PUS-B event identifiers

The v1.2 `PusB` `eventID` field was part of a project-specific secondary header. v2.0 does not assign that field an ECSS meaning automatically. Migration should choose one of:

1. encode the event identifier in the application/service data defined by the mission's PUS service;
2. preserve a proprietary wire field through a clearly named custom secondary-header implementation.

A proprietary event field should not be relabeled as a standards PUS field without a normative mapping.

## Secondary-header API names

| v1.2 | v2.0 |
|---|---|
| `setDataFieldHeader(...)` | `setSecondaryHeader(...)` |
| `getDataFieldHeader()` | `getSecondaryHeader()` |
| `getDataFieldHeaderBytes()` | `getSecondaryHeaderBytes()` |
| `getDataFieldHeaderFactory()` | `getSecondaryHeaderFactory()` |
| `getDataFieldHeaderFlag()` | `getSecondaryHeaderFlag()` |

Packet-level secondary-header presence is normally derived from the installed object.

## Packet ownership and direction

v2.0 uses `ccsds::PacketDirection` for generic directional headers. PUS revision/direction come from the concrete `rev_a`/`rev_c` plus `TcHeader`/`TmHeader` type. Installing a directional header synchronizes the primary-header Packet Type.

Packet error control remains generic Packet policy:

```cpp
packet.setPacketErrorControlMode(
  ccsds::PacketErrorControlMode::None);
```

## Checked finalization and serialization

| v1.2 API | v2.0 API |
|---|---|
| `Packet::serialize()` byte vector | `ccsds::ResultBuffer` |
| `Packet::update()` unreported finalization | `ccsds::ResultBool` |
| `DataField::serialize()` byte vector | `ccsds::ResultBuffer` |
| `Manager::getPacketsBuffer()` byte vector | `ccsds::ResultBuffer` |

All call sites should check the returned Result before accessing bytes.

## Validator

v2.0 uses named checks:

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);

if (report.failed(ccsds::ValidationCode::Crc16)) {
  handleBadCrc();
}
```

Template comparison covers Packet Identification, segmentation class, Packet-level PEC, and secondary-header contract/tailoring. `ValidationReport` has fixed storage and is available under `CCSDS_MCU`.

## Manager migration

A v2.0 Manager uses a complete Packet template as its generation and receive schema:

```cpp
ccsds::Packet packetTemplate;
packetTemplate.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});
packetTemplate.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>(tailoring));

ccsds::Manager manager;
manager.setPacketTemplate(packetTemplate);
```

One Manager represents one Packet Identification and one sequence stream. Raw pointer-plus-size overloads are available for application data, packet ingestion, and stream loading. Const Manager objects expose template, packet collection, and Validator references through `getTemplateReference()`, `getPacketsReference()`, and `getValidatorReference()`.

## Deserialization

PUS parsing can use a preinstalled schema:

```cpp
packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>());
packet.deserialize(wire);
```

or typed parsing:

```cpp
packet.deserialize<ccsds::pus::rev_c::TmHeader>(wire);
```

The equivalent typed pointer-plus-size path is available through `ccsds::buffer`.

## Configuration migration

v2.0 PUS identity is selected by `secondary_header_type`.

### Generic

```ini
ccsds_packet_error_control:string=crc16
ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_APID:int=42
ccsds_segmented:bool=false
define_secondary_header:bool=false
```

### PUS-A TC

```ini
ccsds_version_number:int=0
ccsds_APID:int=42
ccsds_segmented:bool=false
define_secondary_header:bool=true
secondary_header_type:string=PUS:revA:TC
pus_source_id_octets:int=1
pus_service_type:uint=17
pus_service_subtype:uint=1
pus_acknowledgement_flags:uint=9
pus_source_id:uint=1
```

### PUS-A TM

```ini
ccsds_version_number:int=0
ccsds_APID:int=42
ccsds_segmented:bool=false
define_secondary_header:bool=true
secondary_header_type:string=PUS:revA:TM
pus_destination_id_octets:int=1
pus_a_tm_packet_subcounter_present:bool=true
pus_packet_subcounter:uint=1
pus_service_type:uint=3
pus_service_subtype:uint=25
pus_destination_id:uint=1
```

### PUS-C TC

```ini
ccsds_version_number:int=0
ccsds_APID:int=42
ccsds_segmented:bool=false
define_secondary_header:bool=true
secondary_header_type:string=PUS:revC:TC
pus_service_type:uint=17
pus_service_subtype:uint=1
pus_acknowledgement_flags:uint=9
pus_source_id:uint=0x1234
```

### PUS-C TM

```ini
ccsds_version_number:int=0
ccsds_APID:int=42
ccsds_segmented:bool=false
define_secondary_header:bool=true
secondary_header_type:string=PUS:revC:TM
pus_service_type:uint=3
pus_service_subtype:uint=25
pus_message_type_counter:uint=1
pus_destination_id:uint=0x1234
pus_time_reference_status:uint=0
```

When CUC is enabled, add the `pus_time_*` fields documented in [CONFIG.md](CONFIG.md).

Configuration keys representing the v1 project-specific PUS layouts or the intermediate profile model are rejected by v2.0 rather than silently reinterpreted.

## CLI migration

The executable names remain `ccsds_encoder`, `ccsds_decoder`, and `ccsds_validator`. Their configuration must use the v2 Packet template schema and canonical PUS selectors. Packet error control can be selected consistently through configuration or `--packet-error-control crc16|none`.

Validator diagnostics are named structured checks rather than positional report fields.

## Package and ABI migration

v2.0.0 uses project version `2.0.0` and shared-library SOVERSION `2`. Installed consumers should rebuild against the v2 headers/library and may require the major version explicitly:

```cmake
find_package(CCSDSPack 2.0 CONFIG REQUIRED)
target_link_libraries(app PRIVATE ccsdspack::CCSDSPack)
```

A v1 binary should not be assumed ABI-compatible with the v2 shared library.

## Hosted and MCU builds

The protocol API remains C++17. `CCSDSPACK_BUILD_MCU=ON` builds Packet, Manager, PUS codecs/tailoring, CUC time, Result/Error, raw-buffer adapters, and Validator as a static library while excluding hosted Config and command-line programs. `-fno-exceptions -fno-rtti` is supported.

The complete library is not claimed to be allocation-free; the fixed-capacity guarantee applies to `ValidationReport`.

## Wire-format impact

Generic Space Packet behavior from v1.2, including corrected Packet Data Length, CRC coverage, bounded parsing, full APID support, sequence rollover, and Idle Packet validation, remains the packet foundation of v2.0.

The v1.2 project-specific PUS-like secondary headers are not wire-compatible aliases for the standards-oriented v2 PUS codecs. Stored/transmitted packets using those layouts must be interpreted according to their original schema and regenerated according to the intended v2 PUS or custom-header schema.

For packet-wire changes that occurred before v1.2, consult the historical v1.2 behavior/release documentation rather than treating them as v1.2-to-v2 changes.
