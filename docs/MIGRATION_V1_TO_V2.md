# Migrating CCSDSPack v1 to v2

CCSDSPack v2 intentionally breaks source compatibility. It removes
project-specific formats that could be mistaken for ECSS PUS revisions and
makes packet finalization, mission tailoring, validation, and time encoding
explicit.

## Namespace migration

The public C++ namespace is lowercase in v2:

```cpp
// v1
CCSDS::Packet packet;

// v2
ccsds::Packet packet;
```

There is no compatibility alias. CMake package and target names remain
`CCSDSPack` and `ccsdspack::CCSDSPack`.

Standards-defined PUS codecs use revision namespaces:

| Removed v1 concept | v2 type |
|---|---|
| `PusA` | `ccsds::pus::rev_a::TcHeader` or `ccsds::pus::rev_a::TmHeader` |
| `PusB` | No replacement; no standards-facing PUS-B revision exists |
| `PusC` | `ccsds::pus::rev_c::TcHeader` or `ccsds::pus::rev_c::TmHeader` |
| mixed factory | `ccsds::SecondaryHeaderFactory` for custom types plus fixed `ccsds::pus::SecondaryHeaderFactory` |

The canonical string selectors remain `PUS:revA:TC`, `PUS:revA:TM`,
`PUS:revC:TC`, and `PUS:revC:TM`. Custom registration cannot claim the reserved
`PUS:` prefix.

## Secondary-header API naming

| Removed name | v2 name |
|---|---|
| `setDataFieldHeader(...)` | `setSecondaryHeader(...)` |
| `getDataFieldHeader()` | `getSecondaryHeader()` |
| `getDataFieldHeaderBytes()` | `getSecondaryHeaderBytes()` |
| `getDataFieldHeaderFactory()` | `getSecondaryHeaderFactory()` |
| `getPusDataFieldHeaderFactory()` | `getPusSecondaryHeaderFactory()` |
| `getDataFieldHeaderFlag()` | `getSecondaryHeaderFlag()` |
| `setDataFieldHeaderFlag(...)` | `setSecondaryHeaderFlag(...)` |

The configuration key is `ccsds_secondary_header_flag`; the former
`ccsds_data_field_header_flag` is rejected.

## Checked serialization

v1 used an empty byte vector as the only finalization-failure signal. v2 returns
the existing exception-free result types:

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

The v1/v1.2 Validator exposed a boolean result and a positional six-element
`std::vector<bool>` report. Code had to know that, for example, one index meant
CRC and another meant sequence continuity.

v2 replaces those positional report semantics with named checks:

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);

if (report.failed(ccsds::ValidationCode::Crc16)) {
  handleBadCrc();
}

if (report.failed(ccsds::ValidationCode::PusDirection)) {
  handleWrongPusDirection();
}
```

`ccsds::ValidationReport`:

- uses a fixed `std::array` with capacity for 32 named checks;
- performs no dynamic allocation itself;
- is available in hosted and `CCSDS_MCU` builds;
- requires only C++17;
- does not require RTTI or exceptions;
- contains only checks that were actually performed;
- can be iterated or queried with `contains()`, `passed()`, and `failed()`.

`ccsds::Validator::validate()` does not mutate the packet, mission profile, or
secondary header. The Validator still maintains its own sequence-stream state.
Call `clear()` before reusing it for an unrelated sequence stream.

The `ccsds_validator` executable now delegates packet/profile validation to the
library Validator instead of keeping an independent copy of the validation
rules. See [VALIDATION.md](VALIDATION.md).

## PUS construction and numeric time

Raw timestamp byte vectors are replaced by a numeric CUC value plus an explicit
wire profile:

```cpp
auto profile = ccsds::pus::makeProfile(
  ccsds::pus::Revision::C,
  ccsds::pus::Direction::Telemetry);
profile.destinationIdOctets = 2;
profile.telemetryTimestampPresent = true;
profile.telemetryTimeCode = ccsds::time::Format::Cuc;
profile.telemetryCuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Explicit,
  4,
  2
};

ccsds::Packet packet;
if (const auto result = packet.setMissionProfile(profile); !result) {
  return result.error().code();
}

auto header = std::make_shared<ccsds::pus::rev_c::TmHeader>(
  profile, 3, 25, 1, 0x1234, 0,
  ccsds::time::CucTime{0x01020304, 0xA0B0});
if (const auto result = packet.setSecondaryHeader(std::move(header)); !result) {
  return result.error().code();
}
```

The same profile must be active before parsing:

```cpp
ccsds::Packet decoded;
const auto profileResult = decoded.setMissionProfile(profile);
if (!profileResult) return profileResult.error().code();
const auto consumed = decoded.deserializeBounded(wire.value(), "PUS:revC:TM");
```

The parser does not infer revision, direction, identifier widths, time layout,
epoch, P-field policy, or packet error control from remaining bytes.

## Configuration migration

Every v2 profile declares:

```ini
mission_profile:string=generic
ccsds_packet_error_control:string=crc16
```

PUS profiles additionally declare exact `pus_revision`, `pus_direction`, the
canonical `secondary_header_type`, identifier widths, revision-specific fields,
and `pus_time_*` values for CUC telemetry. Packet, Manager, encoder, decoder, and
validator all use this same profile.

Legacy `pus_version`, `pus_event_id`, `pus_time_code`, and
`secondary_header_type=PusA|PusB|PusC` values fail with a migration error.
Complete v2 profiles are in [`example/config`](../example/config).

## Hosted versus bare-metal use

The public protocol library remains C++17. `ccsds::Packet`, `ccsds::Manager`,
mission profiles, PUS codecs, CUC time, Result types, and the structured Validator
are built into the MCU static library.

`ccsds::Config` and the command-line executables are host-side conveniences and
are excluded when `CCSDSPACK_BUILD_MCU=ON` defines `CCSDS_MCU`.

A typical MCU build can use:

```text
-fno-exceptions -fno-rtti
```

without changing the Validator API.

## Wire-format impact

The removed classes encoded project-specific layouts. Existing legacy packet
bytes must be regenerated with a selected revision, direction, mission profile,
and time layout; renaming a class or selector is insufficient.

The generic Packet Data Length, CRC coverage, bounded parsing, APID-width, and
sequence corrections were already part of v1.2.0. They are retained by v2 and
should not be presented as new v1.2-to-v2 wire-format breaks.
