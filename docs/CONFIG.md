<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Configuration file

[Main README](../README.md) | [PUS tailoring](MISSION_TAILORING.md) | [CLI](CLI.md)

Hosted CCSDSPack configuration files use one typed entry per line:

```ini
<key>:<type>=<value>
```

Lines beginning with `#` are comments. The parser is `ccsds::Config`; configuration parsing is excluded from `CCSDS_MCU` builds.

The schema mirrors the runtime ownership model: generic packet policy configures `ccsds::Packet`, while `secondary_header_type` identifies a concrete secondary header and optional PUS keys configure only its supported fields/tailoring.

## Value types

| Type | Meaning |
|---|---|
| `bool` | `true`, `false`, `1`, or `0` |
| `int` | Signed 32-bit decimal or hexadecimal integer |
| `uint` | Unsigned 64-bit decimal or hexadecimal integer |
| `float` | Floating-point value |
| `string` | Text, optionally quoted |
| `bytes` | Comma-separated octets, e.g. `[0x01, 2, 255]` |

## Generic Packet configuration

```ini
ccsds_packet_error_control:string=crc16
data_field_size:int=1024
ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_APID:int=0x123
ccsds_segmented:bool=false
define_secondary_header:bool=false
```

| Key | Required | Constraint |
|---|---:|---|
| `ccsds_packet_error_control` | optional | `crc16` or `none`; default `crc16` |
| `data_field_size` | optional | content capacity `0..65535` |
| `ccsds_version_number` | yes | `0` |
| `ccsds_type` | for direction-neutral packets | `false` TM, `true` TC |
| `ccsds_APID` | yes | `0..2047` |
| `ccsds_segmented` | yes | initial segmented/unsegmented template state |
| `define_secondary_header` | optional | default `false` |
| `secondary_header_type` | when enabled | canonical PUS selector or registered custom type |
| `application_data` | optional | initial application or Idle Packet data |

The Secondary Header Flag is derived from the installed secondary-header object. `data_field_size` is capacity; Packet Data Length is calculated from the actual serialized content.

## PUS identity

A PUS configuration selects one concrete header:

```ini
define_secondary_header:bool=true
secondary_header_type:string=PUS:revC:TC
```

| Selector | Concrete class | Direction |
|---|---|---|
| `PUS:revA:TC` | `ccsds::pus::rev_a::TcHeader` | Telecommand |
| `PUS:revA:TM` | `ccsds::pus::rev_a::TmHeader` | Telemetry |
| `PUS:revC:TC` | `ccsds::pus::rev_c::TcHeader` | Telecommand |
| `PUS:revC:TM` | `ccsds::pus::rev_c::TmHeader` | Telemetry |

For a directional PUS header, `ccsds_type` may be omitted. If supplied, it must agree with the concrete direction.

Common packet fields:

| Key | Constraint |
|---|---|
| `secondary_header_spare_octets` | optional `0..255`, encoded as zero bytes |
| `pus_service_type` | `0..255` |
| `pus_service_subtype` | `0..255` |

## Telecommand fields

| Key | Applies to | Constraint |
|---|---|---|
| `pus_source_id_octets` | PUS-A TC | `0`, `1`, `2`, or `4` |
| `pus_acknowledgement_flags` | PUS-A/C TC | `0..15` |
| `pus_source_id` | PUS-A/C TC | must fit active width |

PUS-C TC source ID width is fixed at two octets.

## Telemetry fields

| Key | Applies to | Constraint |
|---|---|---|
| `pus_destination_id_octets` | PUS-A TM | `0`, `1`, `2`, or `4` |
| `pus_a_tm_packet_subcounter_present` | PUS-A TM | optional boolean tailoring |
| `pus_packet_subcounter` | PUS-A TM | `0..255` when present |
| `pus_message_type_counter` | PUS-C TM | `0..65535` |
| `pus_time_reference_status` | PUS-C TM | `0..15` |
| `pus_destination_id` | PUS-A/C TM | must fit active width |
| `pus_time_format` | PUS-A/C TM | `none` or `cuc` |

PUS-C TM destination ID width is fixed at two octets.

## CUC telemetry time

```ini
pus_time_format:string=cuc
pus_time_epoch:string=ccsds-1958-tai
pus_time_p_field:string=explicit
pus_time_coarse_octets:int=4
pus_time_fine_octets:int=2
pus_time_coarse:uint=0x01020304
pus_time_fine:uint=0xA0B0
```

| Key | Constraint |
|---|---|
| `pus_time_epoch` | `ccsds-1958-tai` or `agency-defined` |
| `pus_time_p_field` | `implicit` or `explicit` |
| `pus_time_coarse_octets` | `1..4` |
| `pus_time_fine_octets` | `0..3` |
| `pus_time_coarse` | must fit configured coarse width |
| `pus_time_fine` | must fit configured fine width |

## Packet error control

`ccsds_packet_error_control` configures the enclosing Packet independently of the selected secondary-header type. In `crc16` mode the CCSDSPack CRC16 trailer is appended at Packet level.

## Idle Packets

APID `0x7FF` identifies Idle Packets. A valid Idle Packet configuration has no secondary header and provides non-empty mission-defined `application_data`. CCSDSPack validates the structure, not the fill pattern.

Complete configurations are maintained under [`example/config`](../example/config).

Upgrade-specific configuration-key mapping is documented in [MIGRATION_V1_TO_V2.md](MIGRATION_V1_TO_V2.md).
