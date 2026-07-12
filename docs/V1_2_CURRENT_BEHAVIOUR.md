# CCSDSPack v1.2 current behaviour

[Main README](../README.md)

> [!NOTE]
> This document describes the behaviour currently implemented on the `develop` branch for the
> upcoming v1.2.0 release. It is a pre-release implementation record, not a claim that every
> v1.2.0 compliance work item is already complete.

## Protocol scope

CCSDSPack v1.2 targets the CCSDS 133.0-B-2 Space Packet Protocol packet data unit within the
library's documented profile.

The following are not implemented by the Space Packet core:

- CCSDS transfer frames;
- attached synchronization markers as a CCSDS packet field;
- CFDP;
- ECSS Packet Utilisation Standard secondary headers.

`CCSDS::Manager` can prepend a configurable synchronization pattern when reading or writing a
packet stream. That pattern is a CCSDSPack container/framing convenience and is not part of the
serialized CCSDS Space Packet.

## Packet serialization

A serialized packet is composed as follows:

```text
6-byte primary header
+ optional secondary header
+ application data
+ optional packet error-control field
```

The 16-bit Packet Data Length field is encoded as:

```text
number of octets following the primary header - 1
```

The encoded length therefore includes:

- the secondary header, when present;
- application data;
- the packet error-control field, when enabled.

A packet data field of 65,536 octets is represented by `0xFFFF`. A larger packet data field cannot
be represented and the existing v1 `Packet::serialize()` API returns an empty buffer. Serialization
also returns an empty buffer when the selected packet error-control mode is `None` and the packet
data field contains no octets.

## Packet error control

The public `PacketErrorControlMode` supports:

- `PacketErrorControlMode::None`;
- `PacketErrorControlMode::CRC16`.

`CRC16` remains the default for existing v1 constructors, configuration paths, and callers.

Configuration files may select the mode with:

```ini
ccsds_packet_error_control:string=crc16
```

or:

```ini
ccsds_packet_error_control:string=none
```

The accepted values are `crc16`, `CRC16`, `none`, and `None`.

When CRC16 is enabled, the calculation covers:

```text
primary header
+ secondary header, when present
+ application data
```

The CRC field itself is excluded. The two CRC bytes are serialized most-significant byte first.
The polynomial, initial value, and final XOR value remain configurable through the C++ API.

## Parsing and validation

`Packet::deserialize()` uses the packet error-control mode configured by the caller. It does not
infer CRC presence from trailing bytes. In CRC16 mode, the final two supplied bytes are extracted as
the received CRC field. In `None` mode, no bytes are removed as packet error control.

The low-level `Packet::deserialize()` overloads currently consume the complete supplied buffer. They
do not yet constrain parsing to the Packet Data Length declared by the primary header. Exact bounded
low-level parsing is tracked by issue #48.

CRC comparison is currently performed by `CCSDS::Validator`. Automatic CRC rejection inside the
low-level packet parsing path is tracked by issue #51.

`CCSDS::Manager::load()` uses the declared Packet Data Length to split concatenated packet streams:

```text
total packet size = 6 + Packet Data Length + 1
```

It rejects truncated primary headers, truncated packet bodies, and incomplete synchronization
patterns before copying packet bytes.

## Manager ownership model

`CCSDS::Manager` represents one packet stream defined by one `Packet` template and one primary-header
identity. In particular, one Manager instance is intended to manage one APID and one associated
sequence-count stream.

Packets generated through `Manager::setApplicationData()` inherit the APID and other primary-header
identity fields from the configured template. A per-APID counter map inside one Manager is therefore
not part of the library model.

Applications handling multiple APIDs should use either:

- one `CCSDS::Manager` instance per APID; or
- independent `CCSDS::Packet` objects when Manager-level segmentation and stream handling are not
  required.

The current implementation does not yet reject every externally constructed packet with an APID
that differs from the Manager template. Enforcing the single-APID invariant for `addPacket()` and
`load()` is tracked by issue #53.

The `data_field_size` template/configuration value is used by Manager as the maximum application-data
chunk placed in each generated packet. It is not copied directly into the encoded Packet Data Length;
the encoded value is derived from the actual serialized packet data field and selected error-control
mode.

## Sequence-count behaviour

The final v1.2 sequence-count policy is not complete yet and remains tracked by issue #52.

Current behaviour includes:

- Manager maintains one sequence counter for its single managed packet stream;
- generated segmented packets receive first, continuation, and last sequence flags;
- clearing Manager packets resets the Manager counter;
- packet header serialization masks the sequence count to the CCSDS 14-bit field.

The current packet update path still forces unsegmented packets to sequence count zero. Preserving
explicit non-zero unsegmented counts, defining modulo-16384 rollover, and making automatic update
policy explicit remain v1.2 work.

## APID behaviour

The primary-header representation contains the CCSDS 11-bit APID field. Direct header assignment
currently masks values to 11 bits.

The configuration loading path does not yet provide complete 11-bit APID handling, and silent field
masking is still present in low-level setters. Full APID range support and explicit validation are
tracked by issues #54 and #55.

## Legacy secondary headers

The v1 `PusA`, `PusB`, and `PusC` classes remain available for source and configuration compatibility.
Their layouts are project-specific legacy secondary headers and are not claimed to implement an ECSS
PUS revision.

Standards-oriented ECSS PUS support remains v2.0.0 scope. The corrected Space Packet length and
packet error-control semantics described in this document are v1.2.0 work, not deferred v2 work.

## Compatibility with releases before v1.2.0

The v1 public method names and signatures remain available, but corrected packet bytes are
intentionally wire-incompatible with packets emitted by earlier versions where:

- Packet Data Length stored the raw data-field size instead of `N - 1`;
- CRC bytes were excluded from Packet Data Length;
- CRC16 was calculated over the data field only rather than from the first primary-header byte.

Users exchanging stored or transmitted packets with pre-v1.2.0 software must account for that wire
format correction.

## Conformance and regression evidence

The current behaviour is covered by:

- `test/src/testGroupEdgeCases.cpp` for independent Packet Data Length and CRC vectors, CRC-disabled
  packets, configurable CRC parameters, maximum length, and overflow rejection;
- `test/src/testGroupValidator.cpp` for validator length and CRC behaviour;
- `test/src/testGroupManagement.cpp` for segmentation, packet-stream boundaries, truncation, sync
  patterns, and file round trips;
- pull request #96 for optional packet error control;
- pull request #97 for corrected Packet Data Length and CRC coverage.
