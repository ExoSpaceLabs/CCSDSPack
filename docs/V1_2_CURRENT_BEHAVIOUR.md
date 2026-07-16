<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v1.2 current behavior

[Main README](../README.md) | [CCSDS 133.0-B-2 EC2 profile](CCSDS_133_0_B_2_PROFILE.md)

## Release scope

CCSDSPack v1.2 implements a Space Packet PDU profile based on **CCSDS 133.0-B-2, Issue 2, including Editorial Change 2 (September 2024)**.

The implemented scope includes:

- six-octet Space Packet primary headers;
- Packet Data Field construction and parsing;
- optional secondary headers;
- application data;
- exact Packet Data Length handling;
- bounded transactional parsing;
- optional CCSDSPack CRC16 mission-profile trailers;
- Packet Sequence Count handling and segmentation utilities;
- Manager Packet Identification binding;
- negative, regression, golden-vector, CLI, and external-consumer tests.

The conformity claim is limited to the **Space Packet PDU profile**. The library does not implement the complete abstract Packet Service, Octet String Service, all protocol procedures, managed parameters, or a PICS.

The following are outside this packet-core scope:

- transfer frames;
- Attached Synchronization Markers as a CCSDS packet field;
- CFDP;
- official ECSS Packet Utilisation Standard secondary headers.

`CCSDS::Manager` can prepend a configurable synchronization pattern around packet streams. That pattern is a CCSDSPack container/framing convenience and is not part of a serialized CCSDS Space Packet.

## Primary header

The primary header is always six octets and contains:

```text
Packet Version Number:       3 bits
Packet Type:                 1 bit
Secondary Header Flag:       1 bit
APID:                       11 bits
Sequence Flags:              2 bits
Packet Sequence Count:      14 bits
Packet Data Length:         16 bits
```

All fields are serialized in network byte order.

### Packet Version Number

`CCSDS::Header` retains the complete three-bit representation as a low-level field container. `CCSDS::Packet` is the Space Packet profile gate:

- parsing rejects any Packet Version Number other than `000`;
- serialization rejects any Packet Version Number other than `000`;
- configuration requires `ccsds_version_number:int=0`.

### Packet Type

Packet Type `0` and `1` are available for telemetry and telecommand packets respectively.

### APID and Idle Packets

The complete 11-bit APID range is supported:

- `0..2046`: normal APIDs;
- `2047` (`0x7FF`): reserved Idle Packet APID.

An Idle Packet is accepted or serialized only when:

- Secondary Header Flag is zero;
- no secondary-header object is installed;
- at least one octet of mission-defined idle user data is present.

CCSDSPack does not validate the mission-specific idle fill pattern.

### Sequence control

Sequence Flags use the CCSDS values:

```text
00 continuation
01 first
10 last
11 unsegmented
```

Manager automatic mode:

- owns one Packet Sequence Count stream;
- assigns one count to every generated packet;
- advances once for every packet, including unsegmented packets;
- wraps modulo 16384;
- assigns first, continuation, last, or unsegmented flags according to generated packet count.

CCSDSPack uses Packet Sequence Count semantics for telemetry and telecommand packets. The optional telecommand Packet Name interpretation is not implemented.

Manual sequence mode reuses the caller-selected count and leaves continuity responsibility to the application.

## Packet Identification and Manager ownership

One `CCSDS::Manager` represents one complete Packet Identification value and one sequence-count stream.

The bound identifier contains:

```text
Packet Version Number
+ Packet Type
+ Secondary Header Flag
+ APID
```

Sequence Flags, Packet Sequence Count, and Packet Data Length are not part of the binding because they vary within the stream.

A Manager:

- binds immediately from a template;
- otherwise binds from the first accepted packet;
- rejects packets with a different identifier;
- loads packet vectors and concatenated buffers transactionally;
- removes the binding on `clear()`.

Applications handling multiple identifiers use multiple Manager instances or independent Packet objects. Within one managed data path, one APID should have one sequence-count authority.

## Packet Data Field and Packet Data Length

A serialized packet contains:

```text
6-octet Packet Primary Header
+ Packet Data Field
```

The Packet Data Field contains:

```text
optional secondary-header bytes
+ application-data bytes
+ optional CCSDSPack CRC16 trailer
```

Packet Data Length is encoded as:

```text
number of Packet Data Field octets - 1
```

The encoded value therefore includes all optional CRC trailer octets.

The representable Packet Data Field range is 1 through 65,536 octets. The total serialized packet range is 7 through 65,542 octets.

`Packet::getSerializedSize()` returns the exact current size as `std::size_t`. The legacy `getFullPacketLength()` API returns `std::uint16_t` and saturates at `UINT16_MAX` instead of wrapping for packets larger than 65,535 octets.

Serialization returns an empty vector when:

- the header is invalid;
- Packet Version Number is not zero;
- Idle Packet structural rules are violated;
- the Packet Data Field would be empty;
- the Packet Data Field would exceed 65,536 octets;
- automatic finalization cannot complete.

## CCSDSPack CRC16 mission profile

CCSDS 133.0-B-2 does not define a third standardized Packet Error Control field. CCSDSPack instead reserves the final two Packet Data Field octets when `PacketErrorControlMode::CRC16` is selected.

The supported modes are:

- `PacketErrorControlMode::CRC16`;
- `PacketErrorControlMode::None`.

CRC16 remains the default for existing v1 constructors and configuration paths.

Configuration files may select:

```ini
ccsds_packet_error_control:string=crc16
```

or:

```ini
ccsds_packet_error_control:string=none
```

In CRC16 mode, the calculation covers:

```text
six-octet primary header
+ optional secondary-header bytes
+ application-data bytes
```

The two CRC octets are excluded from their own calculation and are serialized most-significant byte first. Polynomial, initial value, and final XOR remain configurable through the C++ API.

The receiving application must configure the expected mode before deserialization. CRC presence is not inferred from packet bytes.

## Parsing and validation

`Packet::deserializeBounded()`:

- requires at least six primary-header octets;
- rejects non-zero Packet Version Numbers;
- derives the exact boundary as `6 + Packet Data Length + 1`;
- rejects truncated packet bodies;
- validates the selected CRC16 trailer when enabled;
- validates Idle Packet structural rules;
- returns the consumed-byte count;
- leaves concatenated packets or unrelated trailing bytes unconsumed;
- stages state and commits only after every check succeeds.

The legacy vector-based `deserialize()` overloads use the same bounded parser and preserve their existing `ResultBool` signatures.

Typed secondary-header parsing requires either:

- a registered fixed-size secondary-header type;
- an explicit byte count for opaque or project-specific variable-size headers.

`CCSDS::Manager::load()` uses Packet Data Length to split concatenated streams and rejects truncated primary headers, packet bodies, and synchronization patterns.

The standalone validator reports length, CRC, version, APID, Packet Type, Secondary Header Flag, Sequence Flags, and sequence-count continuity separately.

## Non-mutating inspection

Packet, Header, and DataField getters do not invoke finalization.

Inspection does not recalculate or alter:

- Packet Data Length;
- CRC16;
- Packet Sequence Count;
- secondary-header contents;
- received packet bytes.

`update()` and `serialize()` are the explicit finalization paths. Manager byte getters serialize packet copies rather than mutating stored packets.

## Secondary headers

The v1 API supports:

- `BufferHeader` for opaque bytes;
- custom registered classes derived from `SecondaryHeaderAbstract`;
- legacy project-specific `PusA`, `PusB`, and `PusC` classes.

The bundled Pus-named classes are retained for source and configuration compatibility. They are not claimed to implement an official ECSS PUS revision.

The variable `PusC` byte sequence described as a time-code field is not automatically validated as a CCSDS or ECSS time-code format.

Standards-oriented ECSS PUS support remains v2.0.0 scope.

## Configuration profile

Required packet keys include:

```ini
ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_data_field_header_flag:bool=false
ccsds_APID:int=291
ccsds_segmented:bool=false
```

`ccsds_version_number` accepts only zero. APID accepts values from 0 through 2047. Idle Packet configurations require no secondary header and non-empty `application_data`.

Optional packet error control:

```ini
ccsds_packet_error_control:string=crc16
```

or:

```ini
ccsds_packet_error_control:string=none
```

The `data_field_size` value controls the Manager application-data chunk capacity. It is not copied directly into Packet Data Length, which is always derived from the finalized serialized Packet Data Field.

## Compatibility with releases before v1.2

The public v1 method names and construction paths remain available. Corrected packet bytes and stricter parsing are intentionally incompatible with some packets generated by earlier releases.

Relevant changes include:

- Packet Data Length stores `N - 1`;
- optional CRC trailer octets are included in Packet Data Length;
- CRC coverage begins at the first primary-header octet;
- low-level parsing honors the declared packet boundary;
- CRC mismatches fail during parsing;
- non-zero Packet Version Numbers are rejected by Packet paths;
- Idle Packet structure is validated;
- sequence counts advance and roll over consistently;
- Manager enforces the complete Packet Identification value.

Stored or transmitted pre-v1.2 packets should be regenerated or migrated explicitly. They should not be assumed to parse under the corrected profile.

## Conformance and regression evidence

The v1.2 behavior is covered by:

- independent Python-generated vectors under `test/reference` and `test/test_resources`;
- exact encode and logical decode assertions;
- malformed length, CRC, version, identifier, sequence, and mutation tests;
- dedicated CCSDS PDU-profile and Idle Packet tests;
- maximum Packet Data Length and exact serialized-size tests;
- Manager segmentation, rollover, concatenation, synchronization-pattern, and transactional-load tests;
- the installed external consumer under `test/package_tester/shared_lib`;
- encoder, decoder, and validator integration tests;
- Linux and Windows CI workflows;
- Doxygen generation checks for public API contracts.
