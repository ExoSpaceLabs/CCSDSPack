# CCSDSPack v2 Mission Tailoring

## Purpose

ECSS-E-ST-70-41C is deliberately mission-tailorable. CCSDSPack v2 therefore does not pretend that one hard-coded secondary-header layout is universally compliant. A mission profile records every selected field width and optional behavior needed to encode and parse packets deterministically.

The initial C++ contract is declared in `inc/CCSDSV2MissionProfile.h`. Phase 3 will add validation and codec integration.

## Initial profile fields

| Field | Meaning | Initial representation |
|---|---|---|
| PUS revision | Standards revision used by the PUS codec | `PusRevision::C` only for v2.0.0 |
| Source-ID width | Number of octets used for the source identifier where applicable | `sourceIdOctets` |
| Destination-ID width | Number of octets used for the destination identifier where applicable | `destinationIdOctets` |
| Packet error control | Whether a packet has no error-control field or a CRC-16-CCITT field | `packetErrorControl` |
| TM timestamp presence | Whether the telemetry secondary header carries a timestamp | `telemetryTimestampPresent` |
| TM time-code family | Selected CCSDS time-code family | `telemetryTimeCode` |
| TM time-code length | Total encoded timestamp length in octets | `telemetryTimeCodeOctets` |

Widths are expressed in octets. A zero width disables an optional identifier only where the selected packet definition permits absence. The profile is not inferred from packet size.

## Required behavior

A standards-facing v2 encoder shall require an explicit mission profile before finalization. A standards-facing v2 parser shall receive the applicable mission profile from the caller or a higher-level routing context.

The parser shall not:

- guess whether a CRC is present from trailing bytes;
- infer a timestamp format from the number of bytes left;
- infer packet direction from a PUS class name;
- reinterpret unknown layouts as PUS-C;
- silently truncate identifiers to fit the configured width.

## Validation rules

The profile validator introduced in Phase 3 shall reject at least the following combinations:

1. any PUS revision other than `PusRevision::C` in the v2.0.0 standards-facing API;
2. `telemetryTimestampPresent == false` with a non-`None` time-code family;
3. `telemetryTimestampPresent == false` with a non-zero time-code length;
4. `telemetryTimestampPresent == true` with `TimeCodeFormat::None`;
5. `telemetryTimestampPresent == true` with a zero time-code length;
6. a time-code length that is invalid for the selected supported CCSDS time-code configuration;
7. identifier widths that cannot represent the supplied source or destination identifier;
8. identifier widths unsupported by the selected PUS-C packet layout;
9. an unknown packet error-control mode;
10. packet construction whose total packet-data field exceeds the CCSDS Packet Data Length representation.

Validation shall return an error. It shall not normalize an invalid profile into a different valid profile.

## Initial supported selections

The Phase 0 contract intentionally permits the following selections while deferring exact codec limits to Phase 3:

- PUS revision: PUS-C only;
- packet direction: telemetry or telecommand, modeled separately from revision;
- packet error control: none or CRC-16-CCITT;
- telemetry time-code family: none, CUC, CDS, or CCS;
- configurable source- and destination-ID widths;
- optional telemetry timestamp.

A listed selection means that the profile model can express it. It does not mean the current v1 codec already implements it correctly.

## Default profile

The initial in-code defaults are deliberately conservative and exist for object initialization, not for silently selecting a mission contract:

```cpp
CCSDS::v2::MissionProfile profile;
```

This yields:

- PUS-C;
- one-octet source identifier;
- one-octet destination identifier;
- CRC-16-CCITT packet error control;
- no telemetry timestamp;
- no time-code family;
- zero timestamp octets.

Public v2 configuration and CLI paths shall serialize every profile choice explicitly, even when it equals the initial default.

## Configuration migration target

The final Phase 6 configuration schema shall contain fields equivalent to:

```text
pus_revision = "C"
packet_direction = "telecommand" | "telemetry"
source_id_octets = <integer>
destination_id_octets = <integer>
packet_error_control = "none" | "crc16-ccitt"
telemetry_timestamp_present = true | false
telemetry_time_code = "none" | "cuc" | "cds" | "ccs"
telemetry_time_code_octets = <integer>
```

Legacy values such as `secondary_header = "PusB"` shall fail with a migration error rather than being assigned an invented standards meaning.

## Ownership of tailoring decisions

CCSDSPack validates and applies a mission profile. It does not decide which PUS services, identifier widths, or time format a mission should use. Those selections belong in the mission's space-to-ground interface control documentation.

## Change control

Adding a new profile field requires all of the following:

- a normative or mission-interface reason;
- an explicit unit and valid range;
- validation rules;
- serialization and parsing behavior;
- positive and negative tests;
- an update to `docs/CCSDS_COMPLIANCE.md`.
