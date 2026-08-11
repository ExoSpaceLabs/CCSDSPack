# Migrating CCSDSPack v1 to v2

CCSDSPack v2 intentionally breaks source compatibility. It removes project-specific secondary-header formats that could be mistaken for standards PUS revisions and replaces duplicated profile state with concrete Packet and secondary-header ownership.

## Namespace migration

The public C++ namespace is lowercase in v2:

```cpp
// v1
CCSDS::Packet packet;

// v2
ccsds::Packet packet;
```

There is no compatibility alias. CMake package and target names remain `CCSDSPack` and `ccsdspack::CCSDSPack`.

## PUS type migration

| Removed concept | v2 type |
|---|---|
| `PusA` | `ccsds::pus::rev_a::TcHeader` or `ccsds::pus::rev_a::TmHeader` |
| `PusB` | No replacement; no standards-facing PUS-B revision exists |
| `PusC` | `ccsds::pus::rev_c::TcHeader` or `ccsds::pus::rev_c::TmHeader` |
| mixed factory | `ccsds::SecondaryHeaderFactory` for custom types plus fixed `ccsds::pus::SecondaryHeaderFactory` |

Canonical runtime/config selectors are `PUS:revA:TC`, `PUS:revA:TM`, `PUS:revC:TC`, and `PUS:revC:TM`. Custom registration cannot claim the reserved `PUS:` prefix.

Revision and direction are intrinsic to the concrete PUS class. `pus::Direction` is replaced by the generic `ccsds::PacketDirection` where direction must be discussed outside the PUS type itself.

## No MissionProfile runtime layer

The final v2 API does not expose `MissionProfile` or `Packet::setMissionProfile()`.

The ownership split is:

- `ccsds::Packet` owns generic Packet policy such as primary-header state, `PacketErrorControlMode`, and CRC parameters;
- the installed concrete secondary-header object owns its own type and optional layout tailoring;
- PUS revision/direction come from `rev_a`/`rev_c` and `TcHeader`/`TmHeader`;
- `ccsds::Manager` uses its complete Packet template as the generation and receive contract.

This removes contradictory states such as “profile says Telecommand, concrete class is `TmHeader`.”

## PUS construction

Normal PUS construction no longer requires a profile object.

```cpp
ccsds::Packet packet;

packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});

packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TcHeader>(
    17, 1, 0x1234, 0x09));
```

Installing the `TcHeader` synchronizes the CCSDS secondary-header flag and Packet Type to Telecommand. A `TmHeader` similarly synchronizes Packet Type to Telemetry.

PUS-C source/destination identifier widths fixed by the supported standards layout are not arbitrary configuration fields: PUS-C TC source ID and PUS-C TM destination ID are two octets.

## Optional PUS tailoring

Only actual mission-selectable layout choices require a tailoring object.

Example PUS-C TM with CUC time:

```cpp
ccsds::pus::rev_c::TmTailoring tailoring;
tailoring.timestampPresent = true;
tailoring.cuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Explicit,
  4,
  2
};

auto header = std::make_shared<ccsds::pus::rev_c::TmHeader>(
  tailoring,
  3, 25, 1, 0x1234, 0,
  ccsds::time::CucTime{0x01020304, 0xA0B0});

packet.setSecondaryHeader(header);
```

PUS-A exposes optional source/destination identifier widths and the optional TM packet subcounter through `rev_a::TcTailoring` / `rev_a::TmTailoring`.

## PUS deserialization

Two equivalent styles are available.

Install a header prototype first:

```cpp
ccsds::Packet packet;
packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>());

const auto result = packet.deserialize(wire);
```

Or use the typed convenience API:

```cpp
ccsds::Packet packet;
const auto result =
  packet.deserialize<ccsds::pus::rev_c::TmHeader>(wire);
```

If the wire layout needs optional tailoring:

```cpp
const auto result =
  packet.deserialize<ccsds::pus::rev_c::TmHeader>(wire, tailoring);
```

The same typed form is available through `ccsds::buffer` for pointer-plus-size input. Parsing checks the CCSDS primary-header Packet Type against the intrinsic direction of the supplied header type.

## Packet error control

`ccsds::PacketErrorControlMode::{CRC16,None}` is generic Packet policy. It works with no secondary header, a custom secondary header, or PUS.

```cpp
packet.setPacketErrorControlMode(
  ccsds::PacketErrorControlMode::None);
```

There is no second PUS CRC. In CRC16 mode the CCSDSPack trailer is appended at Packet level and participates in Packet Data Length.

## Secondary-header API naming

| Removed name | v2 name |
|---|---|
| `setDataFieldHeader(...)` | `setSecondaryHeader(...)` |
| `getDataFieldHeader()` | `getSecondaryHeader()` |
| `getDataFieldHeaderBytes()` | `getSecondaryHeaderBytes()` |
| `getDataFieldHeaderFactory()` | `getSecondaryHeaderFactory()` |
| `getPusDataFieldHeaderFactory()` | `getPusSecondaryHeaderFactory()` |
| `getDataFieldHeaderFlag()` | `getSecondaryHeaderFlag()` |
| `setDataFieldHeaderFlag(...)` | `setSecondaryHeaderFlag(...)` at primary-header level when manually needed |

At Packet level, secondary-header presence is normally derived from the installed object rather than configured independently.

## Checked serialization

v1 used an empty byte vector as the only finalization-failure signal. v2 uses exception-free Result types:

| v1 API | v2 API |
|---|---|
| `std::vector<std::uint8_t> Packet::serialize()` | `ccsds::ResultBuffer Packet::serialize()` |
| `void Packet::update()` | `ccsds::ResultBool Packet::update()` |
| `std::vector<std::uint8_t> DataField::serialize()` | `ccsds::ResultBuffer DataField::serialize()` |
| `std::vector<std::uint8_t> Manager::getPacketsBuffer()` | `ccsds::ResultBuffer Manager::getPacketsBuffer()` |

```cpp
const auto wire = packet.serialize();
if (!wire) {
  log(wire.error().code(), wire.error().message());
  return wire.error().code();
}
send(wire.value());
```

## Validator migration

The old positional boolean report is replaced with named `ValidationCode` checks in a fixed-capacity `ValidationReport`.

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);

if (report.failed(ccsds::ValidationCode::Crc16)) {
  handleBadCrc();
}

if (report.failed(ccsds::ValidationCode::PusTailoring)) {
  handleBadPusLayout();
}
```

`ValidationReport` uses a fixed `std::array`, performs no dynamic allocation itself, is available under `CCSDS_MCU`, and requires neither RTTI nor exceptions.

Template comparison now checks Packet Identification, segmentation class, packet-level PEC, and the installed secondary-header contract/tailoring. There is no separate profile comparison.

See [VALIDATION.md](VALIDATION.md).

## Raw-buffer and embedded integration

The vector API remains supported. Pointer-plus-size callers can inspect the declared packet size and parse directly from transport-owned buffers:

```cpp
const auto declared = ccsds::buffer::declaredPacketSize(header, 6U);

const auto consumed = ccsds::buffer::deserializeBounded(
  packet, rxBuffer, receivedBytes);
```

Typed PUS raw parsing is also available:

```cpp
const auto consumed =
  ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TcHeader>(
    packet, rxBuffer, receivedBytes);
```

Manager accepts raw application/packet/stream buffers and exposes const-reference inspection APIs. In v2.0 the raw adapters still bridge through vector-backed internals; this is an API boundary for future zero-copy/heap-free work, not a claim that the complete library is allocation-free.

See [RAW_BUFFERS.md](RAW_BUFFERS.md).

## Configuration migration

Generic Packet settings and PUS identity/tailoring are separate.

Generic example:

```ini
ccsds_packet_error_control:string=crc16
ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_APID:int=42
ccsds_segmented:bool=false
define_secondary_header:bool=false
```

PUS example:

```ini
ccsds_packet_error_control:string=crc16
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

The selector supplies revision and direction. `mission_profile`, `pus_revision`, and `pus_direction` are not part of the final v2 schema. The older `pus_version`, `pus_event_id`, `pus_time_code`, and `secondary_header_type=PusA|PusB|PusC` forms are also obsolete.

See [CONFIG.md](CONFIG.md) and [`example/config`](../example/config).

## Hosted versus bare-metal use

The public protocol library remains C++17. `ccsds::Packet`, `ccsds::Manager`, concrete PUS codecs, PUS tailoring structs, CUC time, Result types, raw-buffer adapters, and the structured Validator are built into the MCU static library.

`ccsds::Config` and command-line executables are host-only conveniences and are excluded when `CCSDSPACK_BUILD_MCU=ON` defines `CCSDS_MCU`.

A typical MCU build can use `-fno-exceptions -fno-rtti` without changing the protocol API.

## Wire-format impact

The removed v1 PUS-like classes encoded project-specific layouts. Existing legacy packet bytes must be regenerated using an explicit standards PUS concrete type and any required tailoring; renaming a class or selector is insufficient.

The generic Packet Data Length, CRC coverage, bounded parsing, APID-width, and sequence corrections were already part of v1.2.0. They are retained by v2 and should not be presented as new v1.2-to-v2 wire-format breaks.
