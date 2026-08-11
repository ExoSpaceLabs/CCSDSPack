<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDS 133.0-B-2 EC2 Space Packet PDU profile

[Main README](../README.md) | [Compliance matrix](../CCSDS_COMPLIANCE.md) | [Structured validation](VALIDATION.md)

## Normative baseline

CCSDSPack v2.0.0 targets **CCSDS 133.0-B-2, Issue 2, June 2020, including Editorial Change 2 of September 2024** for its supported Space Packet PDU profile.

The claim covers packet construction, checked serialization, bounded transactional parsing, inspection, sequence-count handling, segmentation support, structured validation, and Manager stream handling for the implemented PDU profile. It does not claim a complete Space Packet Protocol entity, complete abstract service interfaces, transfer frames, routing, COP-1, CFDP, or a completed PICS.

## Primary header

CCSDSPack implements the fixed six-octet primary header:

- Packet Version Number: 3 bits, required to be `000`;
- Packet Type: 1 bit, telemetry `0` or telecommand `1`;
- Secondary Header Flag: 1 bit;
- APID: 11 bits;
- Sequence Flags: 2 bits;
- Packet Sequence Count: 14 bits;
- Packet Data Length: 16 bits.

A directional secondary header exposes `ccsds::PacketDirection`. Installing such a header synchronizes Packet Type with the header direction. Direction-neutral custom headers leave Packet Type under caller control.

## APID and Idle Packets

The complete APID range `0..2047` is supported. APID `0x7FF` identifies an Idle Packet. A valid Idle Packet has no secondary header and contains at least one octet of mission-defined idle user data. CCSDSPack validates these structural conditions but does not define the mission fill pattern.

## Sequence control

Sequence Flags use the CCSDS values:

| Bits | Meaning |
|---|---|
| `00` | continuing segment |
| `01` | first segment |
| `10` | last segment |
| `11` | unsegmented packet |

Manager automatic sequencing advances once per generated packet and wraps modulo 16384. Validator can independently check legal segmentation transitions and sequence-count continuity for one sequence stream.

## Packet Data Length and size

Packet Data Length is:

```text
number of octets following the six-octet primary header - 1
```

Therefore:

```text
total packet size = 6 + Packet Data Length + 1
```

The Packet Data Field range is 1 through 65,536 octets, giving a total packet range of 7 through 65,542 octets. `Packet::getSerializedSize()` reports the exact complete size as `std::size_t`.

## Secondary headers

CCSDSPack supports:

- opaque `BufferHeader` bytes;
- user-registered custom secondary-header classes;
- `ccsds::pus::rev_a::TcHeader` and `TmHeader`;
- `ccsds::pus::rev_c::TcHeader` and `TmHeader`.

Concrete PUS types own revision and direction. Optional wire-layout choices are held by direction-specific tailoring structs. PUS-C TC source ID and TM destination ID are fixed at two octets; PUS-A exposes the applicable optional identifier widths.

See [PUS tailoring](MISSION_TAILORING.md).

## Packet error control

CCSDS 133.0-B-2 defines the Space Packet as the Primary Header plus Packet Data Field; it does not define a third standardized packet error-control field.

CCSDSPack therefore treats error control as explicit Packet-level policy:

- `PacketErrorControlMode::None`: no trailer is reserved;
- `PacketErrorControlMode::CRC16`: the final two Packet Data Field octets contain CRC-16/CCITT-FALSE.

In CRC16 mode the trailer contributes to Packet Data Length, is encoded most-significant byte first, and is excluded from its own CRC calculation. The CRC covers the six-byte primary header, secondary-header bytes, and application data. Parsing validates the selected CRC mode transactionally.

## Bounded parsing

`deserializeBounded()`:

- validates the six-byte primary header and Packet Version Number;
- derives the exact packet boundary from Packet Data Length;
- rejects truncated bodies;
- validates CRC16 when enabled;
- parses the supplied secondary-header schema;
- returns the number of consumed octets;
- does not absorb following packets or unrelated trailing bytes;
- commits decoded state only after the operation succeeds.

For PUS, the schema can be supplied by a preinstalled concrete header, typed `Packet::deserialize<HeaderT>()`, a typed raw-buffer overload, or a canonical runtime selector.

## Manager stream model

One `ccsds::Manager` represents one complete Packet Identification value and one Packet Sequence Count stream. Its Packet template carries:

- Packet Version Number;
- Packet Type;
- Secondary Header Flag/secondary-header object;
- APID;
- packet-level error-control mode;
- concrete secondary-header type and optional tailoring.

Sequence Flags, Packet Sequence Count, Packet Data Length, and application data vary across generated packets. Applications managing multiple identifiers use multiple Manager instances or independent Packet objects.

The optional Manager synchronization pattern is external stream framing and is not part of the CCSDS Space Packet PDU.

## Structured validation

`ccsds::Validator` returns a fixed-capacity `ValidationReport` containing named checks. Generic checks cover primary-header/version, Packet Data Length, CRC16 when enabled, secondary-header presence/direction, segmentation, sequence continuity, Packet Identification, template PEC, and template secondary-header contract.

PUS checks cover concrete revision/direction, Packet Type, tailoring, encoded size, reserved/version bits, spare fields, acknowledgement/source ID, destination ID, PUS-A TM packet subcounter state, PUS-C TM time-reference status, and active CUC timestamp fit.

See [VALIDATION.md](VALIDATION.md).

## Conformance evidence

The current integration candidate is exercised by 125 native tests, independent generic and PUS byte vectors, malformed/negative inputs, CLI integration, installed-package consumers and examples, Linux/Windows CI, Doxygen, package/cross-build generation, and a Cortex-M compile/link probe.

Physical arm64 and STM32 execution and dedicated fuzz/sanitizer CI remain separate v2.0.0 release-acceptance gates until recorded.
