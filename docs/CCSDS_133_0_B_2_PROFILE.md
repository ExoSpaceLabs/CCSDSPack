<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDS 133.0-B-2 EC2 Space Packet PDU profile

[Main README](../README.md) | [v2 compliance baseline](CCSDS_COMPLIANCE.md) | [Structured validation](VALIDATION.md)

## Normative baseline

CCSDSPack v2 targets the Space Packet Protocol packet data unit defined by:

- **CCSDS 133.0-B-2, Issue 2, June 2020**;
- including **Editorial Change 2, September 2024**.

Editorial Change 2 changes document presentation only. It does not alter the Space Packet wire format or protocol semantics.

## Conformity claim

The supported claim is:

> CCSDSPack v2 implements a CCSDS 133.0-B-2 EC2 Space Packet PDU profile.

This claim covers construction, serialization, bounded parsing, structured
validation, sequence-count handling, and stream management for the implemented
Space Packet PDU profile.

It does **not** claim implementation of the complete Space Packet Protocol entity,
including all abstract service interfaces, sending/receiving procedures, managed
parameters, or a completed PICS. Transfer frames, COP-1, CFDP, and other protocol
layers are outside this library.

`ccsds::Manager` may prepend a configurable synchronization pattern around packet
streams. That pattern is a CCSDSPack framing convenience and is not part of a
CCSDS Space Packet.

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
`ccsds::Header` remains a low-level field container; `ccsds::Packet` is the v2
Space Packet profile gate.

### Packet Type

Both telemetry (`0`) and telecommand (`1`) packet types are supported. For a PUS
profile, Packet Type must agree with the explicit `ccsds::pus::Direction`.

### APID and Idle Packets

The complete 11-bit APID range is supported. APID `0x7FF` is reserved for Idle
Packets. A serializable or accepted Idle Packet must:

- encode Secondary Header Flag `0`;
- contain no secondary-header object;
- use a generic mission profile;
- carry at least one octet of mission-defined idle user data.

CCSDSPack validates these structural rules but not the mission-specific fill pattern.

### Sequence control

Sequence Flags use the CCSDS values:

| Bits | Meaning |
|---|---|
| `00` | continuing segment |
| `01` | first segment |
| `10` | last segment |
| `11` | unsegmented packet |

The Manager uses one modulo-16384 Packet Sequence Count stream per bound Packet
Identification value. Automatic mode advances once per generated packet,
including unsegmented packets.

`ccsds::Validator` can independently validate legal segmentation transitions and
sequence-count continuity. It retains its own sequence-stream state and advances
that state only after the enabled checks for the packet pass.

CCSDSPack uses Packet Sequence Count semantics for both telemetry and telecommand
packets. The optional telecommand Packet Name interpretation is not implemented.

## Packet Data Length and size

Packet Data Length is encoded as:

```text
number of octets following the six-octet primary header - 1
```

Therefore:

```text
total packet size = 6 + Packet Data Length + 1
```

The representable Packet Data Field range is 1 through 65,536 octets, giving a
total packet-size range of 7 through 65,542 octets.

Use `ccsds::Packet::getSerializedSize()` for the complete range. The legacy
`getFullPacketLength()` returns `std::uint16_t` and saturates at `UINT16_MAX`.

## Secondary headers

CCSDS 133.0-B-2 permits optional secondary-header information. CCSDSPack supports:

- opaque `BufferHeader` bytes;
- user-registered secondary-header classes;
- standards-oriented `ccsds::pus::rev_a::TcHeader`;
- `ccsds::pus::rev_a::TmHeader`;
- `ccsds::pus::rev_c::TcHeader`;
- `ccsds::pus::rev_c::TmHeader`.

The v1 project-specific `PusA`, `PusB`, and `PusC` classes are removed. There is
no PUS-B revision. PUS-A and PUS-C use explicit revision/direction mission
profiles and canonical selectors.

PUS telemetry time is numeric and profile-driven. The supported basic CUC codec
records the CCSDS-1958 or agency-defined epoch, implicit or explicit one-octet
P-field, and 1-4 coarse plus 0-3 fine octets. Calendar conversion, leap-second
handling, and agency-epoch definition are outside the packet codec.

## CCSDSPack CRC16 mission profile

CCSDS 133.0-B-2 defines a Space Packet as:

```text
Packet Primary Header + Packet Data Field
```

It does not define a third standardized packet error-control field.

When `PacketErrorControlMode::CRC16` is selected, CCSDSPack reserves the final
two octets of the Packet Data Field for a mission-profile CRC-16/CCITT-FALSE
trailer. Those two octets:

- are included in Packet Data Length;
- are serialized most-significant byte first;
- are excluded from their own CRC calculation;
- are validated during parsing and can be reported by the structured Validator.

The CRC input is the six-octet primary header followed by secondary-header and
application-data bytes.

`PacketErrorControlMode::None` reserves no trailer octets. The receiving side
must select the expected mode explicitly.

## Parsing profile

`deserializeBounded()`:

- validates the six-octet primary header;
- rejects non-zero Packet Version Numbers;
- derives the exact boundary from Packet Data Length;
- rejects truncated packet bodies;
- validates the configured CRC trailer when enabled;
- parses the selected PUS secondary header when a PUS profile is active;
- returns the number of consumed octets;
- leaves later packets or unrelated trailing bytes unconsumed;
- commits parsed state only after parsing succeeds.

Malformed wire bytes can therefore fail before a `Packet` exists for structured
object validation.

## Structured Validator profile

`ccsds::Validator` returns a fixed-capacity `ccsds::ValidationReport` containing
named `ValidationCode` checks rather than positional boolean indices.

Generic checks cover primary-header/version, Packet Data Length, CRC16 when
applicable, secondary-header presence, sequence state, and optional template
Packet Identification/profile checks.

PUS profiles additionally validate revision, direction, Packet Type,
mission-profile equality, secondary-header size, version/reserved bits, spare
fields, acknowledgement/source ID, destination ID, PUS-A TM packet-subcounter
policy, PUS-C TM time-reference status, and configured CUC timestamp fit.

The report itself uses `std::array`, performs no dynamic allocation, and remains
available in C++17 `CCSDS_MCU` builds with exceptions and RTTI disabled. See
[VALIDATION.md](VALIDATION.md).

## Manager profile

One `ccsds::Manager` represents one complete Packet Identification value and one
sequence-count stream. The bound identifier contains:

- Packet Version Number;
- Packet Type;
- Secondary Header Flag;
- APID.

Sequence Flags, Packet Sequence Count, and Packet Data Length vary within the stream.
Applications managing multiple identifiers use multiple Manager instances or
independent Packet objects.

## Compatibility

v2 does not retain the v1 namespace or legacy PUS source API. It preserves the
corrected v1.2 generic packet-wire behaviour, including Packet Data Length,
optional CRC coverage, exact packet-boundary parsing, sequence rollover, complete
Packet Identification enforcement, Packet Version Number validation, and Idle
Packet validation.

Those generic corrections are historical v1.2 behaviour, not new v1.2-to-v2
wire breaks. Stored packets using the removed project-specific secondary headers
must be regenerated with an explicit v2 mission profile.

## Conformance evidence

The v2 profile is exercised by:

- inherited independent generic Space Packet vectors and regression tests from v1.2;
- revision-specific PUS-A/PUS-C fixed and independent vectors;
- positive and negative mission-profile and CUC tests;
- structured Validator tests for generic and PUS checks;
- installed-package consumer and CLI integration tests;
- Linux and Windows CI.

Historical Raspberry Pi 5 and STM32H755 results from v1.2 remain regression
references. v2 arm64 and Cortex-M7 execution are repeated and recorded as
separate v2 release gates before tagging v2.0.0.

UML diagram generation is a manual documentation utility and is not part of the
v2.0.0 conformance or release gate.
