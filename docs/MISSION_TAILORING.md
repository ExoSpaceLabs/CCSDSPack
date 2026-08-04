# CCSDSPack v2 Mission Tailoring

## Purpose

CCSDS packet error control and the ECSS PUS-A/PUS-C secondary-header layouts contain mission-selected parameters. CCSDSPack v2 records those choices explicitly so packet encoding and parsing remain deterministic.

The public contract and validation API are declared in `inc/CCSDSMissionProfile.h`.

## Generic and PUS profiles

A default-constructed `MissionProfile` represents a generic CCSDS Space Packet:

```cpp
CCSDS::MissionProfile profile;
```

The default does not silently enable PUS. This is required because generic CCSDS packets must remain usable without selecting a PUS revision.

To construct a PUS packet, callers explicitly select:

- `pusEnabled = true`;
- `pusRevision = PusRevision::A` or `PusRevision::C`;
- `direction = PacketDirection::Telecommand` or `PacketDirection::Telemetry`;
- the applicable identifier widths;
- packet error control;
- telemetry time configuration where applicable.

PUS revision and packet direction are independent concepts. TC and TM are distinct layouts for each supported revision.

## Initial profile fields

| Field | Meaning |
|---|---|
| `pusEnabled` | Selects generic CCSDS or the PUS-C profile |
| `pusRevision` | Standards revision used by the PUS codec: `A` or `C` |
| `direction` | Telecommand or telemetry, independent from PUS revision |
| `sourceIdOctets` | Number of source-ID octets where the selected TC layout requires them |
| `destinationIdOctets` | Number of destination-ID octets where the selected TM layout requires them |
| `packetErrorControl` | Common packet-layer `PacketErrorControlMode` used by `CCSDS::Packet` |
| `telemetryTimestampPresent` | Whether a TM secondary header includes a timestamp |
| `telemetryTimeCode` | Selected CCSDS time-code family |
| `telemetryTimeCodeOctets` | Total encoded timestamp length until a format-specific profile replaces it |
| `pusATmPacketSubcounterPresent` | Selects the optional PUS-A TM packet subcounter |
| `secondaryHeaderSpareOctets` | Number of zero-valued, octet-aligned spare bytes |

The profile reuses the packet-layer error-control enum. It does not declare a second `PacketErrorControlMode`, because public types are apparently more useful when the compiler sees only one of them.

## Required behaviour

A standards-facing v2 encoder shall use a validated applicable profile before finalization. A standards-facing parser shall receive the applicable profile from the caller or a higher-level routing context.

The parser shall not:

- guess whether a CRC is present from trailing bytes;
- infer a timestamp format from the number of bytes remaining;
- infer packet direction from a class name;
- reinterpret unknown layouts as PUS-C;
- silently truncate identifiers to fit a configured width;
- treat a generic packet as PUS merely because a profile object was default constructed.

## Validation rules

The validator delivered by #67 shall reject at least:

1. `pusEnabled == true` with an unspecified or unsupported revision;
2. PUS construction without explicit direction;
3. TC construction using TM-only destination or timestamp settings where disallowed;
4. TM construction using TC-only acknowledgement or source settings where disallowed;
5. `telemetryTimestampPresent == false` with a non-`None` time-code family;
6. `telemetryTimestampPresent == false` with a non-zero time-code length;
7. `telemetryTimestampPresent == true` with `TimeCodeFormat::None`;
8. `telemetryTimestampPresent == true` with a zero encoded time length;
9. a time configuration invalid for the selected supported CCSDS time representation;
10. identifier widths unable to represent the supplied identifier;
11. identifier widths unsupported by the selected revision/direction layout;
12. an unknown packet error-control mode;
13. packet construction exceeding the CCSDS Packet Data Length representation.

Validation returns an error and does not normalize an invalid profile into a different valid profile.

## Time-code scope

The profile can select CUC or CDS timestamp bytes. The PUS secondary-header codec enforces presence and exact encoded size; epoch conversion, P-field construction, and numeric time conversion remain caller responsibilities.

Only time formats with implemented codecs and independent vectors may appear in the final v2.0.0 support claim.

## Configuration migration target

The final v2 configuration schema shall contain fields equivalent to:

```text
packet_profile = "generic" | "pus"
pus_revision = "A" | "C"
packet_direction = "telecommand" | "telemetry"
source_id_octets = <integer>
destination_id_octets = <integer>
packet_error_control = "none" | "crc16"
telemetry_timestamp_present = true | false
telemetry_time_code = "none" | <supported-format>
telemetry_time_code_octets = <integer>
```

Legacy values such as `secondary_header_type = "PusA"`, `"PusB"`, or `"PusC"` shall fail with a migration error rather than acquiring an invented standards meaning.

## Ownership of tailoring decisions

CCSDSPack validates and applies a mission profile. It does not decide which services, identifier widths, packet error control, epoch, or time representation a mission should use. Those selections belong in the mission interface control documentation.

## Change control

Adding or claiming support for a profile field requires:

- a normative or mission-interface reason;
- explicit units and valid ranges;
- validation rules;
- deterministic serialization and parsing behaviour;
- positive and negative tests;
- independent vectors where wire representation changes;
- an update to `docs/CCSDS_COMPLIANCE.md`.
