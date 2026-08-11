<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Configuration file

[Main README](../README.md) | [Mission tailoring](MISSION_TAILORING.md) | [CLI](CLI.md)

CCSDSPack configuration files use one typed entry per line:

```ini
<key>:<type>=<value>
```

Lines beginning with `#` are comments. The host-only parser is
`ccsds::Config`. Configuration parsing is intentionally excluded from
`CCSDS_MCU`; bare-metal applications construct `ccsds::MissionProfile` and
packet objects directly in C++17.

## Value types

| Type | Meaning |
|---|---|
| `bool` | `true`, `false`, `1`, or `0` |
| `int` | Signed 32-bit decimal or hexadecimal integer |
| `uint` | Unsigned 64-bit decimal or hexadecimal integer |
| `float` | Floating-point value |
| `string` | Text, optionally enclosed in double quotes |
| `bytes` | Comma-separated octets such as `[0x01, 2, 255]` |

Use `uint` for PUS identifiers, counters, and numeric time values so the full
configured wire width is available.

```cpp
ccsds::Config config;
const auto loaded = config.load("mission.cfg");
if (!loaded) return loaded.error().code();

const auto coarse = config.get<std::uint64_t>("pus_time_coarse");
```

## Common packet profile

Every v2 packet configuration explicitly declares its profile and packet-error
control:

```ini
mission_profile:string=generic
ccsds_packet_error_control:string=crc16
data_field_size:int=1024

ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_secondary_header_flag:bool=false
ccsds_APID:int=0x123
ccsds_segmented:bool=false
define_secondary_header:bool=false
```

| Key | Type | Required | Constraint |
|---|---|---:|---|
| `mission_profile` | string | yes | `generic` or `pus` |
| `ccsds_packet_error_control` | string | yes | `crc16` or `none` |
| `data_field_size` | int | optional | Manager application-data capacity, `0..65535` |
| `ccsds_version_number` | int | yes | `0` |
| `ccsds_type` | bool | yes | `false` TM, `true` TC |
| `ccsds_secondary_header_flag` | bool | yes | Must agree with the selected profile/header |
| `ccsds_APID` | int | yes | `0..2047`; `2047` is the Idle Packet APID |
| `ccsds_segmented` | bool | yes | Initial segmented/unsegmented template state |
| `define_secondary_header` | bool | yes for PUS | PUS requires `true` |
| `secondary_header_type` | string | when enabled | Canonical PUS selector or registered custom type |
| `application_data` | bytes | optional | Initial application or Idle Packet data |

`data_field_size` is a capacity setting. Packet Data Length is calculated during
serialization from the bytes actually present.

The CLI framing keys are `sync_pattern_enable`, optional `sync_pattern`, and
`validation_enable` for decoder validation behavior.

## PUS profile

A PUS configuration additionally requires an exact revision, direction, and
canonical selector:

| Key | Type | Applies to | Constraint |
|---|---|---|---|
| `pus_revision` | string | all PUS | `A` or `C` |
| `pus_direction` | string | all PUS | `TC` or `TM` |
| `secondary_header_type` | string | all PUS | `PUS:revA:TC`, `PUS:revA:TM`, `PUS:revC:TC`, or `PUS:revC:TM` |
| `secondary_header_spare_octets` | int | all PUS | `0..255`; encoded spares must be zero |
| `pus_service_type` | uint | all PUS | `0..255` |
| `pus_service_subtype` | uint | all PUS | `0..255` |

### Telecommand fields

| Key | Type | Constraint |
|---|---|---|
| `pus_source_id_octets` | int | `0`, `1`, `2`, or `4`; PUS-C requires `2` |
| `pus_acknowledgement_flags` | uint | four-bit field, `0..15` |
| `pus_source_id` | uint | Must fit `pus_source_id_octets` |

### Telemetry fields

| Key | Type | Constraint |
|---|---|---|
| `pus_destination_id_octets` | int | `0`, `1`, `2`, or `4`; PUS-C requires `2` |
| `pus_time_format` | string | `none` or `cuc` |
| `pus_a_tm_packet_subcounter_present` | bool | Optional for PUS-A TM only |
| `pus_packet_subcounter` | uint | Required when the PUS-A subcounter is present; `0..255` |
| `pus_message_type_counter` | uint | PUS-C TM only; `0..65535` |
| `pus_time_reference_status` | uint | PUS-C TM only; four-bit field, `0..15` |
| `pus_destination_id` | uint | Must fit `pus_destination_id_octets` |

### CUC telemetry time

When `pus_time_format:string=cuc`, all of these keys are required:

| Key | Type | Constraint |
|---|---|---|
| `pus_time_epoch` | string | `ccsds-1958-tai` or `agency-defined` |
| `pus_time_p_field` | string | `implicit` or `explicit` |
| `pus_time_coarse_octets` | int | `1..4` |
| `pus_time_fine_octets` | int | `0..3` |
| `pus_time_coarse` | uint | Integral seconds fitting the coarse width |
| `pus_time_fine` | uint | Binary fractional counter fitting the fine width |

For an explicit P-field, the codec writes and verifies the one-octet basic CUC
preamble. For an implicit P-field, the wire contains only the coarse and fine
T-field counters.

Complete executable configurations are committed in
[`example/config`](../example/config), including PUS-C TM profiles with and
without a CUC time field.

## Validation ownership

Configuration selects the expected wire profile. It does not replace runtime
validation. `ccsds::Packet` validates profile-dependent parsing/finalization and
`ccsds::Validator` reports named generic and PUS checks through
`ccsds::ValidationReport`. See [VALIDATION.md](VALIDATION.md).

## Idle Packets

APID `0x7FF` is reserved for Idle Packets. A valid Idle Packet configuration
uses a generic profile, disables the secondary header, and supplies non-empty
mission-defined `application_data`. CCSDSPack validates the structure but not the
mission-specific fill pattern.

## Removed v1 keys

The v1 `pus_version`, `pus_event_id`, and `pus_time_code` fields and the
`secondary_header_type=PusA|PusB|PusC` categories are rejected. They are not
silently mapped to standards-compliant PUS layouts. See
[MIGRATION_V1_TO_V2.md](MIGRATION_V1_TO_V2.md).
