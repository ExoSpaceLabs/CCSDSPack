<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Configuration file

[Main README](../README.md) | [PUS tailoring](MISSION_TAILORING.md) | [CLI](CLI.md)

CCSDSPack configuration files use one typed entry per line:

```ini
<key>:<type>=<value>
```

Lines beginning with `#` are comments. The host-only parser is `ccsds::Config`.
Configuration parsing is excluded from `CCSDS_MCU`; bare-metal applications
construct `ccsds::Packet`, concrete secondary headers, and optional PUS tailoring
directly in C++17.

There is no `MissionProfile` configuration layer. Generic packet policy belongs
to `Packet`; a concrete PUS selector identifies revision and direction; optional
PUS fields configure only that header's tailoring or packet-specific values.

## Value types

| Type | Meaning |
|---|---|
| `bool` | `true`, `false`, `1`, or `0` |
| `int` | Signed 32-bit decimal or hexadecimal integer |
| `uint` | Unsigned 64-bit decimal or hexadecimal integer |
| `float` | Floating-point value |
| `string` | Text, optionally enclosed in double quotes |
| `bytes` | Comma-separated octets such as `[0x01, 2, 255]` |

Use `uint` for PUS identifiers, counters, and numeric CUC values.

## Generic packet configuration

A generic packet without a directional secondary header can be configured as:

```ini
ccsds_packet_error_control:string=crc16
data_field_size:int=1024
ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_APID:int=0x123
ccsds_segmented:bool=false
define_secondary_header:bool=false
```

| Key | Type | Required | Constraint |
|---|---|---:|---|
| `ccsds_packet_error_control` | string | optional | `crc16` or `none`; default `crc16` |
| `data_field_size` | int | optional | Packet Data Field content capacity, `0..65535` |
| `ccsds_version_number` | int | yes | `0` |
| `ccsds_type` | bool | for direction-neutral packets | `false` TM, `true` TC |
| `ccsds_APID` | int | yes | `0..2047`; `2047` is the Idle Packet APID |
| `ccsds_segmented` | bool | yes | Initial segmented/unsegmented template state |
| `define_secondary_header` | bool | optional | Defaults to `false` |
| `secondary_header_type` | string | when enabled | Canonical PUS selector or registered custom type |
| `application_data` | bytes | optional | Initial application or Idle Packet data |

The CCSDS secondary-header flag is derived from whether a secondary header is
actually installed. It is not a second configuration source of truth.

`data_field_size` is a capacity setting. Packet Data Length is calculated during
serialization from the bytes actually present.

CLI framing keys such as `sync_pattern_enable`, optional `sync_pattern`, and
`validation_enable` remain tool-level settings rather than Packet wire fields.

## PUS identity

A PUS configuration selects one concrete standards-facing secondary-header type:

```ini
define_secondary_header:bool=true
secondary_header_type:string=PUS:revC:TC
```

The selector itself defines revision and direction:

| Selector | Concrete class | Direction |
|---|---|---|
| `PUS:revA:TC` | `ccsds::pus::rev_a::TcHeader` | Telecommand |
| `PUS:revA:TM` | `ccsds::pus::rev_a::TmHeader` | Telemetry |
| `PUS:revC:TC` | `ccsds::pus::rev_c::TcHeader` | Telecommand |
| `PUS:revC:TM` | `ccsds::pus::rev_c::TmHeader` | Telemetry |

`ccsds_type` is therefore optional for these directional PUS headers. If supplied,
it must agree with the concrete header direction. `Packet::setSecondaryHeader()`
synchronizes Packet Type and the secondary-header flag from the installed header.

Common PUS packet fields are:

| Key | Type | Constraint |
|---|---|---|
| `secondary_header_spare_octets` | int | Optional, `0..255`; encoded spares are zero |
| `pus_service_type` | uint | `0..255` |
| `pus_service_subtype` | uint | `0..255` |

## Telecommand fields

| Key | Type | Applies to | Constraint |
|---|---|---|---|
| `pus_source_id_octets` | int | PUS-A TC | Optional tailoring: `0`, `1`, `2`, or `4` |
| `pus_acknowledgement_flags` | uint | PUS-A/C TC | Four-bit field, `0..15` |
| `pus_source_id` | uint | PUS-A/C TC | Must fit the active width |

PUS-C TC source ID width is fixed at two octets by the supported PUS-C layout and
normally is not configured. If the width key is supplied for compatibility, the
loader accepts only `2`.

## Telemetry fields

| Key | Type | Applies to | Constraint |
|---|---|---|---|
| `pus_destination_id_octets` | int | PUS-A TM | Optional tailoring: `0`, `1`, `2`, or `4` |
| `pus_a_tm_packet_subcounter_present` | bool | PUS-A TM | Optional packet-subcounter tailoring |
| `pus_packet_subcounter` | uint | PUS-A TM | Required when the subcounter is enabled; `0..255` |
| `pus_message_type_counter` | uint | PUS-C TM | `0..65535` |
| `pus_time_reference_status` | uint | PUS-C TM | Four-bit field, `0..15` |
| `pus_destination_id` | uint | PUS-A/C TM | Must fit the active width |
| `pus_time_format` | string | PUS-A/C TM | `none` or `cuc` |

PUS-C TM destination ID width is fixed at two octets. As with PUS-C TC, an
explicit width key is unnecessary and, if present, must equal `2`.

## CUC telemetry time

The default TM tailoring has no timestamp. To enable a numeric basic CUC field:

```ini
pus_time_format:string=cuc
pus_time_epoch:string=ccsds-1958-tai
pus_time_p_field:string=explicit
pus_time_coarse_octets:int=4
pus_time_fine_octets:int=2
pus_time_coarse:uint=0x01020304
pus_time_fine:uint=0xA0B0
```

| Key | Type | Constraint |
|---|---|---|
| `pus_time_epoch` | string | `ccsds-1958-tai` or `agency-defined` |
| `pus_time_p_field` | string | `implicit` or `explicit` |
| `pus_time_coarse_octets` | int | `1..4` |
| `pus_time_fine_octets` | int | `0..3` |
| `pus_time_coarse` | uint | Counter fitting the selected coarse width |
| `pus_time_fine` | uint | Counter fitting the selected fine width |

For an explicit P-field, the codec writes and verifies the one-octet basic CUC
preamble. For an implicit P-field, the wire contains only the coarse and fine
T-field counters.

Complete executable configurations are committed in
[`example/config`](../example/config), including PUS-C TM examples with and
without a CUC time field.

## Packet error control ownership

`ccsds_packet_error_control` configures the enclosing CCSDS Packet regardless of
whether the packet has no secondary header, a custom header, or a PUS header.
There is no PUS-specific second CRC setting. In `crc16` mode the optional
CCSDSPack CRC16 trailer is appended at the packet level.

## Validation ownership

Configuration constructs a Packet and, when requested, a concrete secondary
header. Runtime validation then checks the resulting object graph rather than a
parallel profile:

- Packet Version Number, Packet Data Length, Packet Type and PEC;
- secondary-header presence and direction;
- PUS concrete revision/direction and tailoring;
- identifiers, reserved/spare fields, optional counters and CUC values;
- Manager template Packet Identification, PEC, and secondary-header contract.

See [VALIDATION.md](VALIDATION.md).

## Idle Packets

APID `0x7FF` is reserved for Idle Packets. A valid Idle Packet configuration has
no secondary header and supplies non-empty mission-defined `application_data`.
CCSDSPack validates the structural rules but not the mission-specific fill pattern.

## Removed configuration concepts

The runtime/configuration design no longer uses `mission_profile`, `pus_revision`,
or `pus_direction`; the concrete PUS selector carries PUS identity. The older
`pus_version`, `pus_event_id`, `pus_time_code`, and
`secondary_header_type=PusA|PusB|PusC` forms are also obsolete and are not part
of the v2 configuration schema.

See [MIGRATION_V1_TO_V2.md](MIGRATION_V1_TO_V2.md).
