<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDS 133.0-B-2 EC2 Space Packet PDU profile

[Main README](../README.md)

## Normative baseline

CCSDSPack v1.2 targets the Space Packet Protocol packet data unit defined by:

- **CCSDS 133.0-B-2, Issue 2, June 2020**;
- including **Editorial Change 2, September 2024**.

Editorial Change 2 changes document presentation only. It does not alter the Space Packet wire format or protocol semantics.

## Conformity claim

The supported claim is:

> CCSDSPack v1.2 implements a CCSDS 133.0-B-2 EC2 Space Packet PDU profile.

This claim covers construction, serialization, bounded parsing, validation, sequence-count handling, and stream management for CCSDS Space Packet PDUs.

It does **not** claim implementation of the complete CCSDS Space Packet Protocol entity, including:

- the abstract Packet Service interface;
- the abstract Octet String Service interface;
- all sending and receiving procedures;
- all managed parameters;
- a completed Protocol Implementation Conformance Statement (PICS);
- transfer frames, Attached Synchronization Markers, CFDP, or other CCSDS protocol layers.

`CCSDS::Manager` may prepend a configurable synchronization pattern around packet streams. That pattern is a CCSDSPack container convenience and is not part of a CCSDS Space Packet.

## Primary header profile

CCSDSPack uses the fixed six-octet primary header with:

- Packet Version Number: 3 bits;
- Packet Type: 1 bit;
- Secondary Header Flag: 1 bit;
- APID: 11 bits;
- Sequence Flags: 2 bits;
- Packet Sequence Count: 14 bits;
- Packet Data Length: 16 bits.

### Packet Version Number

A serialized or parsed Space Packet must encode Packet Version Number `000`.

`CCSDS::Header` remains a low-level three-bit field container for source compatibility and future protocol work. `CCSDS::Packet` is the Space Packet profile gate and refuses to serialize non-zero versions. Configuration files also require:

```ini
ccsds_version_number:int=0
```

### Packet Type

Both telemetry (`0`) and telecommand (`1`) packet types are supported.

### APID and Idle Packets

The complete 11-bit APID range is supported. APID `0x7FF` is reserved for Idle Packets.

A serializable or accepted Idle Packet must:

- encode Secondary Header Flag `0`;
- contain no secondary-header object;
- carry at least one octet of mission-defined idle user data.

CCSDSPack validates these structural rules but does not validate the mission-specific idle fill pattern.

### Sequence control

Sequence Flags use the CCSDS values:

| Bits | Meaning |
|---|---|
| `00` | continuing segment |
| `01` | first segment |
| `10` | last segment |
| `11` | unsegmented packet |

The Manager uses one modulo-16384 Packet Sequence Count stream per bound Packet Identification value. Automatic mode advances once for every generated packet, including unsegmented packets.

CCSDSPack uses **Packet Sequence Count semantics for both telemetry and telecommand packets**. The optional telecommand Packet Name interpretation is not implemented.

Manual sequence mode is an expert override. Applications using manual counts are responsible for continuity.

## Packet Data Length and size

Packet Data Length is encoded as:

```text
number of octets following the six-octet primary header - 1
```

Therefore:

```text
total packet size = 6 + Packet Data Length + 1
```

The representable Packet Data Field range is 1 through 65,536 octets, giving a total packet-size range of 7 through 65,542 octets.

Use:

```cpp
std::size_t CCSDS::Packet::getSerializedSize() const;
```

for the complete range. The legacy `getFullPacketLength()` returns `std::uint16_t` and saturates at `UINT16_MAX` rather than wrapping when the exact packet size is larger.

## Secondary headers

CCSDS 133.0-B-2 permits optional secondary-header information. CCSDSPack supports:

- opaque `BufferHeader` bytes;
- user-registered secondary-header classes;
- legacy project-specific `PusA`, `PusB`, and `PusC` classes.

The bundled `PusA`, `PusB`, and `PusC` names are retained for v1 compatibility. Their layouts are not claimed to implement an official ECSS Packet Utilisation Standard revision.

The `PusC` byte sequence described as a time-code field is not automatically a conformant CCSDS time code. Mission software must select and validate any required time-code format separately.

## CCSDSPack CRC16 mission profile

CCSDS 133.0-B-2 defines a Space Packet as:

```text
Packet Primary Header + Packet Data Field
```

It does not define a third standardized packet error-control field.

When `PacketErrorControlMode::CRC16` is selected, CCSDSPack reserves the **final two octets of the Packet Data Field** for a mission-profile CRC-16/CCITT-FALSE trailer. Those two octets:

- are included in Packet Data Length;
- are serialized most-significant byte first;
- are excluded from their own CRC calculation;
- are validated during parsing.

The CRC input is:

```text
six-octet Packet Primary Header
+ optional secondary-header bytes
+ application-data bytes
```

`PacketErrorControlMode::None` reserves no trailer octets. CRC16 remains the default for existing v1 construction and configuration paths.

## Parsing profile

`deserializeBounded()`:

- validates the six-octet primary header before copying the body;
- rejects non-zero Packet Version Numbers;
- derives the exact boundary from Packet Data Length;
- rejects truncated packet bodies;
- validates the configured CRC trailer when enabled;
- returns the number of consumed octets;
- leaves later concatenated packets or unrelated trailing bytes unconsumed;
- commits parsed state only after validation succeeds.

The receiving application must configure whether the stream uses the CRC16 profile. The mode is not inferred from packet bytes.

## Manager profile

One `CCSDS::Manager` represents one complete Packet Identification value and one sequence-count stream.

The bound identifier contains:

- Packet Version Number;
- Packet Type;
- Secondary Header Flag;
- APID.

Sequence Flags, Packet Sequence Count, and Packet Data Length are excluded because they vary within the stream.

Applications managing multiple identifiers use multiple Manager instances or independent Packet objects. Within one managed data path, one APID should have one sequence-count authority.

## Compatibility with releases before v1.2

The public v1 source API remains available, but corrected wire behavior may be incompatible with packets generated by earlier versions. In particular, v1.2 corrects:

- Packet Data Length to use `N - 1`;
- inclusion of the optional CRC trailer in Packet Data Length;
- CRC coverage from the first primary-header octet;
- exact packet-boundary parsing;
- sequence-count advancement and rollover;
- full Packet Identification enforcement;
- Packet Version Number and Idle Packet profile validation.

Stored or transmitted pre-v1.2 packets should be regenerated or migrated with an explicit compatibility tool. They should not be assumed to parse under the corrected profile.

## Conformance evidence

The profile is exercised by:

- independent Python-generated byte vectors committed under `test/test_resources`;
- packet encode/decode and maximum-size tests;
- malformed length, CRC, version, identifier, and sequence tests;
- dedicated Idle Packet and PDU-profile tests;
- the installed shared-library consumer under `test/package_tester/shared_lib`;
- Linux and Windows CI builds and execution.
