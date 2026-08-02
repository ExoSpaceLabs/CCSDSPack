<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v1.2 CCSDS Space Packet compliance matrix

## Status

This document is the detailed compliance and traceability matrix for the
CCSDSPack v1.2.0 Space Packet implementation. It shall be read together with:

- [`COMPLIANCE.md`](COMPLIANCE.md), the concise release-facing claim;
- [`docs/CCSDS_133_0_B_2_PROFILE.md`](docs/CCSDS_133_0_B_2_PROFILE.md), the
  supported packet profile and wire behaviour;
- [`docs/V1_2_CURRENT_BEHAVIOUR.md`](docs/V1_2_CURRENT_BEHAVIOUR.md), the
  implementation and compatibility record.

This matrix covers the CCSDS Space Packet layer only. The bundled legacy
`PusA`, `PusB`, and `PusC` classes are deliberately excluded from this matrix
and do not contribute to the v1.2 compliance claim.

The presence of an API, an internal encode/decode round trip, or a similarly
named class is not, by itself, conformance evidence.

## Normative baseline

| Area | Selected baseline | v1.2.0 decision |
|---|---|---|
| CCSDS Space Packet Protocol | CCSDS 133.0-B-2, Recommended Standard Issue 2, June 2020, including Editorial Change 2 of September 2024 | Space Packet PDU construction, serialization, bounded parsing, validation, supported assembly/extraction behaviour, and packet-format parameters |
| Packet Utilisation Standard | Not selected for the v1.2 claim | Legacy secondary-header classes remain available but are outside this compliance matrix |

Where this matrix and an implementation comment disagree, the normative
standard, the concise claim, and this matrix take precedence in that order.

## Conformance claim boundary

CCSDSPack v1.2.0 claims conformance for the supported Space Packet PDU profile
and the library behaviour required to create, serialize, parse, inspect,
validate, segment, and manage those PDUs.

It does **not** claim to implement a complete CCSDS Space Packet Protocol
entity or a complete Protocol Implementation Conformance Statement.

In particular:

- packet-format and codec requirements are in scope;
- the packet assembly and extraction behaviour performed by `Packet`,
  `Manager`, and `Validator` is in scope;
- lower-layer packet transfer is outside the library scope;
- complete abstract `PACKET` and `OCTET_STRING` service primitive APIs are
  outside the claim;
- network routing, transfer frames, virtual channels, COP-1, CFDP, and
  transport bindings are outside the claim;
- the optional CRC16 trailer is a CCSDSPack mission-profile convention, not a
  field defined by CCSDS 133.0-B-2.

A claim of complete protocol-entity conformance would require a completed PICS
covering every applicable mandatory item in annex A.

## Status vocabulary

| Status | Meaning |
|---|---|
| Implemented | Behaviour is present and backed by focused automated or independent evidence. |
| Implemented in profile | The applicable subset used by the v1.2 PDU profile is implemented; the complete abstract protocol service remains outside scope. |
| Mission-tailored | CCSDS permits or requires mission-specific content or policy. |
| Unsupported | Intentionally outside the v1.2 compliance claim. |
| Not applicable | The requirement belongs to a lower or higher layer not implemented by this library. |

## Traceability conventions

- **Direct**: corresponds directly to one or more normative CCSDS requirements.
- **Derived**: required to preserve a directly specified property such as
  self-delimitation or exact length accounting.
- **External**: not specified by CCSDS 133.0-B-2 and selected by the
  CCSDSPack mission profile.
- **Library invariant**: required for deterministic public-library behaviour,
  but not itself a CCSDS wire-format clause.

Test references name the focused repository test source or the fixed independent
evidence used by the release. The complete native suite currently contains
93 passing regression and conformance tests.

## Annex A PICS scope summary

This table records the relationship between the CCSDS 133.0-B-2 PICS proforma
and the v1.2 library claim. It is a scope statement, not a completed
system-level PICS.

| PICS item(s) | CCSDS reference | Capability | v1.2 classification | Status |
|---|---|---|---|---|
| SPP-1 | 3.2.2 | Space Packet service data unit | Represented by `CCSDS::Packet`; complete abstract service API is outside scope | Implemented in profile |
| SPP-2 | 3.2.3 | Octet String service data unit | Complete Octet String Service is outside scope | Unsupported |
| SPP-3, SPP-7 | 3.3.2.2, 3.4.2.2 | APID service parameter | Full 11-bit APID and Idle APID handling are in scope | Implemented |
| SPP-4, SPP-9 | 3.3.2.3, 3.4.2.4 | Packet/Data Loss Indicator | Sequence continuity can be validated; the abstract service indicator is outside scope | Implemented in profile |
| SPP-5 | 3.3.2.4 | Quality of Service | Queueing and transport QoS are outside scope | Unsupported |
| SPP-6 | 3.4.2.1 | Octet String parameter | Bytes may be represented as packet data, but no complete service primitive is provided | Unsupported |
| SPP-8 | 3.4.2.3 | Secondary Header Indicator | Represented by the Secondary Header Flag and configured packet template | Implemented |
| SPP-10 to SPP-13 | 3.3.3.2, 3.3.3.3, 3.4.3.2, 3.4.3.3 | Abstract Packet and Octet String service primitives | Outside the library claim | Unsupported |
| SPP-14 | 4.1 | Space Packet PDU | Core release scope | Implemented |
| SPP-15 | 4.1.3 | Packet Primary Header | Core release scope | Implemented |
| SPP-16 | 4.1.4 | Packet Data Field | Core release scope | Implemented |
| SPP-17 | 4.1.4.2 | Packet Secondary Header | Conditional and mission-tailored where enabled | Implemented |
| SPP-18 | 4.1.4.3 | User Data Field | Conditional packet content | Implemented |
| SPP-19 | 4.2.2 | Packet Assembly Function | Implemented by the supported `Packet` and `Manager` construction paths | Implemented in profile |
| SPP-20 | 4.2.3 | Packet Transfer Function | Lower-layer transport responsibility | Not applicable |
| SPP-21 | 4.3.2 | Packet Extraction Function | Implemented by bounded parsing and validation for the supported profile | Implemented in profile |
| SPP-22 | 4.3.3 | Packet Reception Function | Subnetwork reception and routing are outside scope | Unsupported |
| SPP-23 | table 5-1 | Maximum Packet Length | Full representable PDU size is supported and boundary-tested | Implemented |
| SPP-24 | table 5-1 | Packet Type of outgoing packets | Telemetry and telecommand values are supported | Implemented |
| SPP-25 | table 5-1 | Packet Multiplexing Scheme | Mission transport and queueing policy | Unsupported |
| SPP-26 | table 5-1 | Service Type per APID | Complete service management is outside scope | Unsupported |

## CCSDS Space Packet clause traceability matrix

| ID | CCSDS 133.0-B-2 clause(s) | PICS item(s) | Trace type | Requirement | Status | Implementation reference | Evidence |
|---|---|---|---|---|---|---|---|
| SPP-PDU-001 | 4.1.2.1 | SPP-14, SPP-15, SPP-16 | Direct | A Space Packet contains a contiguous six-octet Packet Primary Header followed by a Packet Data Field of 1 to 65,536 octets. | Implemented | `CCSDS::PrimaryHeader`, `CCSDS::Packet::serialize`, `deserializeBounded` | independent minimum and ordinary packet vectors; `testGroupEdgeCases.cpp`; `testGroupParsing.cpp` |
| SPP-PDU-002 | 4.1.2.2 | SPP-14, SPP-23 | Direct | Total Space Packet size is 7 to 65,542 octets, subject to any smaller configured limit. | Implemented | `Packet::getSerializedSize`; overflow checks during serialization | maximum Packet Data Length and overflow rejection tests |
| SPP-HDR-001 | 4.1.3.1 | SPP-15 | Direct | The Packet Primary Header is mandatory, exactly six octets, and contains version, identification, sequence control, and data length in that order. | Implemented | `CCSDS::PrimaryHeader`; header serialization and parsing | primary-header assignment, exact-vector, and bounded-parse tests |
| SPP-HDR-002 | 4.1.3.2.1, 4.1.3.2.2 | SPP-15 | Direct | Packet Version Number occupies bits 0-2 and is `000` for a Space Packet. | Implemented | packet serialization/configuration/parser gates | non-zero PVN serialization, configuration, and parsing rejection tests; STM32 PVN test |
| SPP-HDR-003 | 4.1.3.3.1.1, 4.1.3.3.1.2 | SPP-15 | Direct | Bits 3-15 contain Packet Type, Secondary Header Flag, and APID. | Implemented | `PrimaryHeader` field packing and unpacking | independent APID/non-zero sequence vector and primary-header range tests |
| SPP-HDR-004 | 4.1.3.3.2.1 to 4.1.3.3.2.3 | SPP-15, SPP-24 | Direct | Packet Type is `0` for telemetry/reporting and `1` for telecommand/requesting. | Implemented | `PrimaryHeader::setType`; packet template | field-boundary and identifier-template tests |
| SPP-HDR-005 | 4.1.3.3.3.1 to 4.1.3.3.3.4; 4.1.4.2.1.2 | SPP-8, SPP-15, SPP-17 | Direct | The Secondary Header Flag reflects actual secondary-header presence and is zero for Idle Packets. | Implemented | `Packet` secondary-header handling and Idle validation | custom secondary-header vector; typed parsing; Idle Packet negative and positive tests |
| SPP-HDR-006 | 4.1.3.3.4.1 to 4.1.3.3.4.4 | SPP-3, SPP-7, SPP-15 | Direct | APID is 11 bits; 0-2046 are mission use and 2047 identifies an Idle Packet. | Implemented | 11-bit APID storage, configuration, and validation | complete APID-range/overflow tests; Idle Packet tests |
| SPP-HDR-007 | 4.1.3.4.1.1, 4.1.3.4.1.2 | SPP-15 | Direct | Bits 16-31 contain Sequence Flags and a 14-bit Packet Sequence Count. | Implemented | `PrimaryHeader` sequence-control packing | independent vector and sequence-count tests |
| SPP-HDR-008 | 4.1.3.4.2.1 to 4.1.3.4.2.3 | SPP-15 | Direct | Sequence Flags encode continuation `00`, first `01`, last `10`, and unsegmented `11`. | Implemented | sequence flag enum, packet generation, segmentation | segmented Manager generation and validator coherence tests |
| SPP-HDR-009 | 4.1.3.4.3.1 to 4.1.3.4.3.4 | SPP-15, SPP-19 | Direct | Packet Sequence Count is preserved, advances continuously when automatic, and rolls over modulo 16,384. | Implemented | `Packet`, `Manager`, `Validator` sequence handling | non-zero unsegmented count, segmented continuity, rollover, parsed preservation, and STM32 Manager tests |
| SPP-HDR-010 | 4.1.3.5.1 to 4.1.3.5.3 | SPP-15 | Direct | Packet Data Length equals the number of octets after the primary header minus one. | Implemented | packet finalization and bounded parser | independent minimum/ordinary/no-CRC/custom-header vectors; length mismatch and overflow tests |
| SPP-DATA-001 | 4.1.4.1.1, 4.1.4.1.2 | SPP-16, SPP-17, SPP-18 | Direct | The Packet Data Field contains at least one octet and comprises optional secondary-header bytes followed by optional user data. | Implemented | `CCSDS::DataField`; packet construction and parsing | minimum packet, custom secondary header, application-data, and empty/invalid construction tests |
| SPP-DATA-002 | 4.1.4.2.1.1 to 4.1.4.2.1.6 | SPP-17 | Direct | A present secondary header immediately follows the primary header, has an integral-octet mission-defined layout, and matches the flag. | Implemented | opaque `BufferHeader` and registered secondary-header factory | custom header independent vector; typed boundary test; segmentation preservation test |
| SPP-DATA-003 | 4.1.4.3 | SPP-18 | Direct | User data follows the optional secondary header and remains within the declared packet boundary. | Implemented | application-data storage and bounded parsing | pointer round trip, large application data, truncation, and separated header/body tests |
| SPP-LEN-001 | 4.1.3.5.2, 4.1.3.5.3; 4.1.4.1.1 | SPP-15, SPP-16 | Direct | Every packet octet after the primary header contributes to Packet Data Length. | Implemented | packet finalization includes secondary header, application data, and selected CRC trailer | CRC coverage, no-CRC length, custom-header, and golden-vector tests |
| SPP-PARSE-001 | 2.1.1; 4.1.2.1; 4.1.3.5 | SPP-14, SPP-21 | Derived | One packet boundary is `6 + Packet Data Length + 1` octets. | Implemented | `Packet::deserializeBounded` | concatenated stream consumes exactly one packet |
| SPP-PARSE-002 | 4.1.2; 4.1.3.5; 4.3.2.2 | SPP-14, SPP-21 | Derived | Input shorter than the declared packet boundary is rejected without committing partial state. | Implemented | bounded transactional parser | truncated body and oversized declared-length tests |
| SPP-PARSE-003 | 2.1.1; 4.1.3.5; 4.3.2.2 | SPP-14, SPP-21 | Derived | Parsing reports consumed octets and does not absorb following packets or trailing bytes. | Implemented | `deserializeBounded` result value | concatenated parsing and legacy first-packet behaviour tests |
| SPP-PROC-001 | 4.2.2.2 to 4.2.2.4 | SPP-19 | Direct | Supported packet assembly creates the primary header, applies secondary-header indication, and applies maintained sequence count. | Implemented in profile | `Packet::serialize`; `Manager::setApplicationData` | Manager creation, segmentation, sequence, identifier, CRC, and STM32 packet-generation tests |
| SPP-PROC-002 | 4.3.2.1, 4.3.2.2 | SPP-21 | Direct | Supported extraction exposes packet components, secondary-header presence, exact boundaries, and sequence continuity information. | Implemented in profile | bounded parser and `Validator` | parsing, decoded-field, CRC, template identity, and sequence-continuity tests |
| SPP-PROC-003 | 4.3.3.1 to 4.3.3.3 | SPP-22 | Direct | Subnetwork reception and APID demultiplexing are performed outside this library. | Unsupported | caller/higher-layer responsibility | explicit claim boundary |
| SPP-MGMT-001 | 5.1; 5.2 table 5-1 | SPP-23, SPP-24, SPP-26 | Direct | Maximum length and outgoing packet type used by the profile are explicit configuration; complete service management remains outside scope. | Implemented in profile | packet template and configuration parser | configuration, field-range, maximum-size, and packet-type tests |
| SPP-API-001 | No direct CCSDS clause | N/A | Library invariant | Inspecting a parsed packet does not mutate received header, sequence, data, or CRC state. | Implemented | const packet getters and explicit finalization | getter non-finalization and parsed inspection preservation tests |

## Packet error-control assumptions

CCSDS 133.0-B-2 does not define a Packet Error Control field or a CRC
algorithm for the Space Packet PDU.

For CCSDSPack v1.2.0:

- packet-error-control mode is selected explicitly as `CRC16` or `None`;
- no CRC presence is inferred from trailing bytes;
- when enabled, the two CRC octets are the final octets of the Packet Data
  Field and contribute to Packet Data Length;
- CRC-16/CCITT-FALSE covers the primary header, optional secondary-header
  bytes, and application-data bytes, excluding the CRC itself;
- CRC bytes are encoded most-significant byte first;
- bounded parsing validates the selected CRC profile transactionally.

| Requirement area | CCSDS relationship | Classification | Status | Evidence |
|---|---|---|---|---|
| Error-control presence | External selection; encoded bytes remain within the CCSDS Packet Data Field | External / mission-tailored | Implemented | CRC16 and CRC-disabled vectors and parser tests |
| CRC coverage | Not defined by CCSDS 133.0-B-2 | External / mission-tailored | Implemented | independent CRC vector, coverage test, and STM32 exact-vector test |
| CRC encoding | Not defined by CCSDS 133.0-B-2 | External / mission-tailored | Implemented | fixed golden vectors and big-endian comparisons |
| CRC verification | Not defined by CCSDS 133.0-B-2 | External / mission-tailored | Implemented | corruption rejection and non-mutating failure tests |

## Evidence baseline

The v1.2 claim is supported by:

- 93 native regression and conformance tests;
- independent Python-generated fixed byte vectors under `test/test_resources`;
- negative tests for invalid version, field ranges, length, CRC, identifier,
  segmentation, sequence continuity, and Idle Packet structure;
- Linux and Windows CI;
- installed-package and exact-version external-consumer tests;
- native Raspberry Pi 5 arm64 package validation ending in
  `CCSDSPACK_AARCH64_TEST:PASS`;
- NUCLEO-H755ZI-Q Cortex-M7 hardware validation ending in
  `CCSDSPACK_MCU_TEST:PASS`.

The hardware runs establish execution on the tested targets. They do not expand
the protocol claim beyond the profile in this matrix.

## Matrix maintenance policy

A row remains **Implemented** only while:

1. its normative or derived requirement remains identified;
2. the cited implementation path remains present;
3. focused positive evidence remains passing;
4. independent expected bytes or calculations remain available for wire
   behaviour;
5. negative evidence remains present for rejected values and malformed input;
6. no contradictory supported configuration path exists.

Changes to packet wire behaviour, the claim boundary, or the selected normative
baseline require this matrix, `COMPLIANCE.md`, and the detailed PDU profile to
be updated together.
