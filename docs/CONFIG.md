<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Configuration file

[Main README](../README.md) | [CCSDS 133.0-B-2 EC2 profile](CCSDS_133_0_B_2_PROFILE.md)

CCSDSPack configuration files use one typed key-value entry per line:

```ini
<variable_name>:<data_type>=<value>
```

Lines beginning with `#` are comments. Integers may use decimal or hexadecimal notation.

## Supported data types

- `bool`: `true` or `false`;
- `int`: decimal or hexadecimal integer;
- `float`: floating-point number;
- `string`: text;
- `bytes`: integer byte array.

Example:

```ini
name:string="packet profile"
integerValue:int=42
hexValue:int=0x6B
enabled:bool=true
ratio:float=0.85
buffer:bytes=[0x01, 0x02, 0x03]
```

## Loading configuration

```cpp
#include <CCSDSPack.h>
#include <iostream>

int main() {
  Config cfg;
  const auto result = cfg.load("/path/to/config.cfg");
  if (!result) {
    std::cerr << result.error().message() << '\n';
    return result.error().code();
  }
  return 0;
}
```

Read a value through `Result<T>`:

```cpp
const auto sizeResult = cfg.get<int>("data_field_size");
if (!sizeResult) return sizeResult.error().code();
const auto dataFieldSize = sizeResult.value();
```

The existing result-assignment macros may also be used where appropriate.

## Packet template configuration

A normal v1.2 packet profile can begin with:

```ini
ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_data_field_header_flag:bool=false
ccsds_APID:int=0x123
ccsds_segmented:bool=false

data_field_size:int=1024
ccsds_packet_error_control:string=crc16

define_secondary_header:bool=false
```

### General settings

| Parameter | Type | Required | Meaning |
|---|---|---:|---|
| `data_field_size` | int | yes | Manager application-data chunk capacity before the optional CRC trailer |
| `sync_pattern_enable` | bool | stream tools | Enables the CCSDSPack stream prefix; it is not part of a CCSDS packet |
| `sync_pattern` | int | when enabled | Four-octet CCSDSPack synchronization pattern |
| `validation_enable` | bool | decoder profile | Enables Manager validation |
| `ccsds_packet_error_control` | string | no | `crc16` or `none`; defaults to `crc16` |

The encoder, decoder, and validator also provide the additive `--packet-error-control` override. See [CLI.md](CLI.md).

### Primary-header settings

| Parameter | Type | Required | Constraint |
|---|---|---:|---|
| `ccsds_version_number` | int | yes | must be `0` for CCSDS 133.0-B-2 Space Packets |
| `ccsds_type` | bool | yes | `false` telemetry, `true` telecommand |
| `ccsds_data_field_header_flag` | bool | yes | indicates optional secondary-header presence |
| `ccsds_APID` | int | yes | `0..2047`; `2047` is the Idle Packet APID |
| `ccsds_segmented` | bool | yes | selects initial segmented or unsegmented template state |

`ccsds_version_number` values from 1 through 7 are rejected by `Packet::loadFromConfig()` even though the low-level Header class retains a three-bit field representation.

CCSDSPack uses Packet Sequence Count semantics for telemetry and telecommand packets. The optional telecommand Packet Name interpretation is not configured or implemented.

## Idle Packet configuration

APID `0x7FF` is reserved for Idle Packets. A valid Idle Packet configuration must:

- set `ccsds_data_field_header_flag` to `false`;
- set `define_secondary_header` to `false`;
- include non-empty mission-defined idle bytes in `application_data`.

Example:

```ini
ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_data_field_header_flag:bool=false
ccsds_APID:int=0x7FF
ccsds_segmented:bool=false

data_field_size:int=16
ccsds_packet_error_control:string=crc16

define_secondary_header:bool=false
application_data:bytes=[0x00]
```

CCSDSPack validates the structure but does not validate the mission-specific idle fill pattern.

## Packet error control profile

Accepted values are:

```ini
ccsds_packet_error_control:string=crc16
```

and:

```ini
ccsds_packet_error_control:string=none
```

In `crc16` mode, CCSDSPack reserves the final two octets of the CCSDS Packet Data Field for a mission-profile CRC-16/CCITT-FALSE trailer. The trailer is included in Packet Data Length. CCSDS 133.0-B-2 does not define it as a separate packet structural field.

The receiving packet, decoder, or validator must be configured with the expected mode before parsing. The mode is not inferred from bytes.

## Secondary-header settings

| Parameter | Type | Required |
|---|---|---:|
| `define_secondary_header` | bool | yes |
| `secondary_header_type` | string | when enabled |
| `pus_version` | int | legacy PusA/PusB/PusC profiles |
| `pus_service_type` | int | legacy PusA/PusB/PusC profiles |
| `pus_service_sub_type` | int | legacy PusA/PusB/PusC profiles |
| `pus_source_id` | int | legacy PusA/PusB/PusC profiles |
| `pus_event_id` | int | legacy PusB only |
| `pus_time_code` | bytes | legacy PusC only |

The built-in `PusA`, `PusB`, and `PusC` layouts are project-specific legacy formats. Their names do not constitute an ECSS PUS compliance claim. `pus_time_code` bytes are not automatically validated as a CCSDS time-code format.

## Packet Data Length

`data_field_size` is a capacity/chunking setting. It is not copied into the primary-header Packet Data Length field.

Packet Data Length is derived during finalization as:

```text
serialized Packet Data Field octets - 1
```

It includes secondary-header, application-data, and optional CRC-trailer octets actually serialized for that packet.
