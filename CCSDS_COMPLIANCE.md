<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v1.2 CCSDS compliance matrix

## Status

This document is the clause-level compliance matrix for the CCSDSPack v1.2.0 Space Packet implementation. It records:

- the selected normative CCSDS baseline;
- the supported protocol and library scope;
- the relationship to the CCSDS 133.0-B-2 Annex A PICS;
- clause-level implementation traceability;
- the evidence supporting the v1.2.0 conformity claim;
- explicit limitations and responsibilities left to the integrating mission system.

The presence of an API, a successful encode/decode round trip, or a familiar class name is not sufficient conformance evidence. Rows are marked **Implemented** only when the behavior is present and covered by focused tests or independently generated expected values.

This matrix covers the CCSDS Space Packet Protocol only. ECSS Packet Utilisation Standard revisions and the legacy `PusA`, `PusB`, and `PusC` formats are intentionally outside this document.

## Normative baseline

| Area | Selected baseline | v1.2.0 decision |
|---|---|---|
| CCSDS Space Packet Protocol | CCSDS 133.0-B-2, Recommended Standard Issue 2, June 2020, including Editorial Change 1 of October 2020 and Editorial Change 2 of September 2024 | Space Packet PDU plus the packet assembly, extraction, validation, and managed packet-format behavior performed by CCSDSPack |

Editorial Changes 1 and 2 do not alter the Space Packet wire format or normative protocol semantics used by this implementation.

## Conformity claim boundary

The supported claim is:

> **CCSDSPack v1.2.0 implements a CCSDS 133.0-B-2 Issue 2, including Editorial Change 2, Space Packet PDU profile for the supported library operations.**

The claim covers the behavior required to create, serialize, parse, inspect, validate, segment, and manage the supported Space Packet PDUs.

It does not claim that CCSDSPack is a complete CCSDS Space Packet Protocol entity implementing every abstract service primitive, every sending or receiving procedure, every managed parameter, or every mandatory item in the Annex A PICS.

In particular:

- packet format and codec requirements are in scope;
- packet assembly and packet extraction behavior performed by the library are in scope;
- packet transfer over a subnetwork is outside the library scope;
- complete abstract `PACKET` and `OCTET_STRING` service primitive APIs are outside the v1.2.0 claim;
- network routing, transfer frames, virtual channels, COP-1, CFDP, and transport bindings are outside the v1.2.0 claim;
- a future claim of complete protocol-entity conformance would require a completed PICS covering every applicable mandatory Annex A item.

## Supported profile

CCSDSPack v1.2.0 supports:

- the fixed six-octet Space Packet Primary Header;
- Packet Version Number `000` enforcement at the `Packet` boundary;
- telemetry and telecommand Packet Types;
- the full 11-bit APID range and Idle Packet structural validation;
- segmented and unsegmented Sequence Flags;
- Packet Sequence Count semantics for telemetry and telecommand packets;
- modulo-16384 automatic sequence-count rollover;
- Packet Data Fields from 1 through 65,536 octets;
- total serialized packet sizes from 7 through 65,542 octets;
- optional mission-defined secondary-header bytes and custom secondary-header codecs;
- application-data bytes;
- exact Packet Data Length calculation;
- bounded transactional parsing with consumed-byte reporting;
- concatenated packet-stream processing;
- explicit CRC16 or no-CRC CCSDSPack packet-error-control profiles;
- deterministic non-mutating packet inspection;
- one complete Packet Identification and one sequence-count stream per `CCSDS::Manager` instance.

The optional telecommand Packet Name interpretation is not implemented. CCSDSPack uses Packet Sequence Count semantics for telecommand packets.

## Status vocabulary

| Status | Meaning |
|---|---|
| Implemented | Behavior is present and supported by focused evidence. |
| Mission-tailored | CCSDS permits or requires mission-specific selection; CCSDSPack provides the mechanism but cannot validate mission policy by itself. |
| Partially implemented | A useful subset exists, but the complete protocol capability is outside the v1.2 claim. |
| Unsupported | Intentionally outside the v1.2.0 claim. |
| Not applicable | The requirement belongs to another system layer rather than this packet library. |

## Traceability conventions

- **Direct**: corresponds directly to one or more normative CCSDS requirements.
- **Derived**: required to preserve a directly specified packet property such as self-delimitation or exact length handling.
- **External**: not defined by CCSDS 133.0-B-2 and selected by the CCSDSPack mission profile or integrating mission.
- **Library invariant**: required for deterministic library behavior but not itself a CCSDS wire-format clause.

## Annex A PICS scope summary

This table explains how the CCSDS 133.0-B-2 Annex A PICS relates to the v1.2.0 library claim. It is a scope map, not a completed system-level PICS.

| PICS item(s) | CCSDS reference | Capability | v1.2.0 classification | Status |
|---|---|---|---|---|
| SPP-1 | 3.2.2 | Space Packet service data unit | Represented through `CCSDS::Packet`; complete abstract service API is not claimed | Implemented for the PDU profile |
| SPP-2 | 3.2.3 | Octet String service data unit | Complete Octet String Service is outside the claim | Unsupported |
| SPP-3, SPP-7 | 3.3.2.2, 3.4.2.2 | APID service parameter | Represented by the 11-bit APID in `Header`, configuration, validation, and Manager binding | Implemented |
| SPP-4, SPP-9 | 3.3.2.3, 3.4.2.4 | Packet/Data Loss Indicator | No abstract service indicator API; sequence continuity can be checked by the validator | Partially implemented |
| SPP-5 | 3.3.2.4 | Quality of Service | Queueing and delivery policy belong to the integrating system | Unsupported |
| SPP-6 | 3.4.2.1 | Octet String parameter | Raw packet data can be carried, but the complete service parameter is not exposed | Unsupported as a service primitive |
| SPP-8 | 3.4.2.3 | Secondary Header Indicator | Represented through the Secondary Header Flag and actual secondary-header presence | Implemented |
| SPP-10 to SPP-13 | 3.3.3.2, 3.3.3.3, 3.4.3.2, 3.4.3.3 | Abstract Packet and Octet String service primitives | Outside the packet-library API claim | Unsupported |
| SPP-14 | 4.1 | Space Packet PDU | Core supported object and wire representation | Implemented |
| SPP-15 | 4.1.3 | Packet Primary Header | Core supported header representation | Implemented |
| SPP-16 | 4.1.4 | Packet Data Field | Core supported data representation | Implemented |
| SPP-17 | 4.1.4.2 | Packet Secondary Header | Supported when selected and defined by the mission | Mission-tailored / Implemented mechanism |
| SPP-18 | 4.1.4.3 | User Data Field | Supported as application-data bytes | Implemented |
| SPP-19 | 4.2.2 | Packet Assembly Function | Performed by `Packet` and `Manager` for the supported profile | Implemented |
| SPP-20 | 4.2.3 | Packet Transfer Function | Lower-layer subnetwork responsibility | Not applicable |
| SPP-21 | 4.3.2 | Packet Extraction Function | Bounded parsing, component extraction, and sequence continuity validation are provided | Implemented for the supported profile |
| SPP-22 | 4.3.3 | Packet Reception Function | `Manager` accepts one bound Packet Identification stream; general APID routing is an application responsibility | Partially implemented |
| SPP-23 | table 5-1 | Maximum Packet Length | Full CCSDS range supported and tested | Implemented |
| SPP-24 | table 5-1 | Packet Type of outgoing packets | Telemetry and telecommand types supported | Implemented |
| SPP-25 | table 5-1 | Packet Multiplexing Scheme | Transport and queueing policy | Unsupported |
| SPP-26 | table 5-1 | Service Type per APID | Complete service management is outside the packet-core claim | Unsupported |

## CCSDS Space Packet clause traceability matrix

| ID | CCSDS 133.0-B-2 clause(s) | PICS item(s) | Trace type | Requirement | v1.2 status | Implementation reference | Evidence |
|---|---|---|---|---|---|---|---|
| SPP-PDU-001 | 4.1.2.1 | SPP-14, SPP-15, SPP-16 | Direct | A Space Packet contains a contiguous six-octet Packet Primary Header followed by a mandatory Packet Data Field of 1 to 65,536 octets. | Implemented | `CCSDS::Header`, `CCSDS::Packet`, `CCSDS::DataField` | minimum and ordinary independent vectors; packet construction and decode tests |
| SPP-PDU-002 | 4.1.2.2 | SPP-14, SPP-23 | Direct | Total Space Packet size is 7 to 65,542 octets, subject to any smaller configured implementation limit. | Implemented | `Packet::getSerializedSize`, packet-size checks in `Packet::update`/`serialize` | `Maximum serialized size is reported without uint16 overflow`; oversize negative tests |
| SPP-HDR-001 | 4.1.3.1 | SPP-15 | Direct | The Packet Primary Header is mandatory, exactly six octets, and contains Version, Identification, Sequence Control, and Data Length in order. | Implemented | `Header::serialize`, `Header::deserialize` | independent golden vectors; exact primary-header field assertions |
| SPP-HDR-002 | 4.1.3.2.1, 4.1.3.2.2 | SPP-15 | Direct | Packet Version Number bits shall be `000` for this Space Packet version. | Implemented at the `Packet` profile boundary | `Packet::serialize`, `Packet::deserializeBounded`, configuration loading | `Packet serialization rejects non-zero Packet Version Number`; configuration and malformed-input tests |
| SPP-HDR-003 | 4.1.3.3.1.1, 4.1.3.3.1.2 | SPP-15 | Direct | Bits 3-15 form Packet Type, Secondary Header Flag, and APID. | Implemented | `Header` field setters/getters and serialization | independent APID/sequence vector and field-layout assertions |
| SPP-HDR-004 | 4.1.3.3.2.1 to 4.1.3.3.2.3 | SPP-15, SPP-24 | Direct | Packet Type is `0` for telemetry/reporting and `1` for telecommand/requesting. | Implemented | `Header::setType`, `PrimaryHeader`, configuration | telemetry/telecommand tests; installed external consumer |
| SPP-HDR-005 | 4.1.3.3.3.1 to 4.1.3.3.3.4; 4.1.4.2.1.2 | SPP-8, SPP-15, SPP-17 | Direct | Secondary Header Flag signals actual secondary-header presence and shall be zero for Idle Packets. | Implemented | `Packet::setDataFieldHeader`, `Packet::update`, Idle validation | secondary-header vectors; both Idle mutation-order tests; received Idle negative test |
| SPP-HDR-006 | 4.1.3.3.4.1 to 4.1.3.3.4.4 | SPP-3, SPP-7, SPP-15 | Direct | APID is 11 bits; 0-2046 are mission APIDs and 2047 identifies an Idle Packet. | Implemented | `Header::setAPID`, configuration, Manager identity | `APID range and header status are handled explicitly`; configuration full-range tests |
| SPP-HDR-007 | 4.1.3.4.1.1, 4.1.3.4.1.2 | SPP-15 | Direct | Bits 16-31 contain two Sequence Flag bits and a 14-bit Packet Sequence Count or Packet Name. | Implemented | `Header` sequence-control serialization | independent vectors and parsed-field assertions |
| SPP-HDR-008 | 4.1.3.4.2.1 to 4.1.3.4.2.3 | SPP-15 | Direct | Sequence Flags encode continuation `00`, first `01`, last `10`, and unsegmented `11`. | Implemented | `Header::setSequenceFlags`, Manager segmentation | segmented and unsegmented Manager tests; exact wire assertions |
| SPP-HDR-009 | 4.1.3.4.3.1, 4.1.3.4.3.2 | SPP-15 | Direct | The 14-bit field carries Packet Sequence Count for telemetry and either count or Packet Name for telecommand; parsed values are preserved. | Implemented for the selected Packet Sequence Count option | `Header`, `Packet`, `Manager` | non-zero unsegmented counts; telecommand count and rollover tests |
| SPP-HDR-010 | 4.1.3.4.3.3 | SPP-15, SPP-19 | Direct | Packet Sequence Counts are independent per APID/user application. | Implemented through one sequence authority per bound Manager stream | `Manager::setPacketTemplate`, complete Packet Identification binding | identifier-isolation tests; documented one-Manager-per-identifier integration rule |
| SPP-HDR-011 | 4.1.3.4.3.4; 4.2.2.4 | SPP-19 | Direct | Automatic Packet Sequence Counts are continuous modulo 16,384. | Implemented | Manager automatic sequence path | rollover vector from 16,383 to 0; segmented and unsegmented progression tests |
| SPP-HDR-012 | 4.1.3.5.1 to 4.1.3.5.3 | SPP-15 | Direct | Packet Data Length equals the number of Packet Data Field octets minus one. | Implemented | `Packet::update`, `Header::setDataLength` | independent CRC16 and no-CRC vectors; length boundary tests |
| SPP-DATA-001 | 4.1.4.1.1, 4.1.4.1.2 | SPP-16, SPP-17, SPP-18 | Direct | Packet Data Field contains an optional Secondary Header followed by an optional User Data Field, with at least one octet present overall. | Implemented | `DataField`, packet finalization checks | empty-field rejection; secondary-header/application-data vectors |
| SPP-DATA-002 | 4.1.4.2.1.1 to 4.1.4.2.1.4 | SPP-17 | Direct | A present Packet Secondary Header immediately follows the primary header, occupies integral octets, and has a mission-known format. | Implemented mechanism / Mission-tailored format | `SecondaryHeaderAbstract`, `SecondaryHeaderFactory`, `BufferHeader` | custom secondary-header golden vector; fixed and variable-length codec tests |
| SPP-DATA-003 | 4.1.4.2.1.5, 4.1.4.2.1.6 | SPP-17 | Direct | Generic secondary-header contents are time information, ancillary data, or both, selected consistently for the managed path. | Mission-tailored | registered secondary-header type and explicit decode length/type | application configuration and integration responsibility; custom header tests verify byte preservation |
| SPP-LEN-001 | 4.1.3.5.2, 4.1.3.5.3; 4.1.4.1.1 | SPP-15, SPP-16 | Direct | Every Space Packet octet after the primary header contributes to Packet Data Length. | Implemented | packet finalization includes secondary header, application data, and selected CRC trailer | `Packet Data Length and CRC16 match independent reference vector`; no-CRC and custom-header vectors |
| SPP-LEN-002 | 4.1.2.2; 4.1.3.5.2, 4.1.3.5.3 | SPP-14, SPP-23 | Direct | Construction rejects sizes not representable by the 16-bit minus-one length field and prevents overflow. | Implemented | packet-size validation and `std::size_t` serialized-size API | maximum-size test; oversize rejection tests |
| SPP-PARSE-001 | 2.1.1; 4.1.2.1; 4.1.3.5.2, 4.1.3.5.3 | SPP-14, SPP-21 | Derived | One packet boundary is `6 + Packet Data Length + 1` octets. | Implemented | `Packet::deserializeBounded` | exact-boundary, concatenated-stream, and trailing-byte tests |
| SPP-PARSE-002 | 4.1.2.1, 4.1.2.2; 4.3.2.2 | SPP-14, SPP-21 | Derived | Input shorter than the declared boundary is rejected without out-of-bounds access. | Implemented | bounded parser staging and validation | short-primary-header and truncated-body negative tests |
| SPP-PARSE-003 | 2.1.1; 4.3.2.2 | SPP-14, SPP-21 | Derived | The parser consumes exactly one packet and reports consumed octets. | Implemented | `Packet::deserializeBounded` result value | concatenated rollover vector; CLI decoder stream tests |
| SPP-PARSE-004 | 2.1.1; 4.1.3.5.2, 4.1.3.5.3 | SPP-14, SPP-21 | Derived | Bytes after the declared boundary are not absorbed into the current packet. | Implemented | bounded parser and Manager stream load | trailing-byte preservation tests; decoder `--trailing-output` integration |
| SPP-PROC-001 | 4.2.2.2 to 4.2.2.4 | SPP-19 | Direct | Packet assembly creates the primary header, maps secondary-header presence, and applies the maintained sequence count. | Implemented for the supported profile | `Packet::update`, `Manager::getPackets`/`getPacketsBuffer` | Manager segmentation, exact bytes, count progression, and CRC tests |
| SPP-PROC-002 | 4.3.2.1, 4.3.2.2 | SPP-21 | Direct | Packet extraction removes the primary header, exposes secondary-header presence, and can evaluate sequence continuity. | Implemented for the supported profile | `Packet::deserializeBounded`, `Validator`, `Manager::load` | parsed-component tests; validator sequence-continuity tests; CLI validator diagnostics |
| SPP-PROC-003 | 4.3.3.1 to 4.3.3.3 | SPP-22 | Direct | Receiving-side code demultiplexes by APID and delivers Packet Service packets intact. | Partially implemented | one `Manager` accepts one bound Packet Identification stream | complete identifier rejection/transactional load tests; general multi-APID routing remains application scope |
| SPP-MGMT-001 | 5.1; 5.2 table 5-1 | SPP-23, SPP-24 | Direct | Maximum Packet Length and outgoing Packet Type are explicit configuration or API selections. | Implemented for supported parameters | configuration loader, Packet and Manager setters | configuration range tests and exact-version external consumer |
| SPP-API-001 | No direct CCSDS clause | N/A | Library invariant | Inspection of a parsed packet does not mutate received fields, length, sequence count, data, or CRC. | Implemented | const getters; explicit `update`/`serialize` finalization paths | non-mutating inspection and repeated serialization tests |

## Packet error-control profile

CCSDS 133.0-B-2 does not define a Packet Error Control field or CRC algorithm for the Space Packet PDU.

CCSDSPack v1.2.0 supports an explicit mission-profile selection:

- `PacketErrorControlMode::CRC16`, the source-compatible v1 default;
- `PacketErrorControlMode::None`.

When enabled, the CRC-16/CCITT-FALSE value occupies the final two octets of the Packet Data Field and therefore contributes to Packet Data Length. The CRC covers the primary header, secondary-header bytes, and application-data bytes, excluding the two CRC octets themselves. It is encoded most-significant byte first.

| Requirement area | CCSDS 133.0-B-2 relationship | Classification | v1.2 status | Evidence |
|---|---|---|---|---|
| Error-control presence | Not defined by CCSDS 133.0-B-2; explicitly selected by the CCSDSPack profile | External / Mission-tailored | Implemented | CRC16 default and no-CRC tests; CLI integration |
| CRC coverage | Not defined by CCSDS 133.0-B-2 | External | Implemented | independent reference vectors covering primary, secondary, and application bytes |
| CRC encoding | Not defined by CCSDS 133.0-B-2 | External | Implemented | exact big-endian vector assertions |
| CRC verification | Not defined by CCSDS 133.0-B-2 | External | Implemented | parse-time mismatch rejection and one-bit corruption tests |

## Evidence summary

### Independent and automated evidence

- independent Python-generated byte vectors under `test/reference` and committed resources under `test/test_resources`;
- 93 native regression and conformance tests;
- negative tests for version, APID, Idle Packet, length, CRC, identifier, sequence, and mutation behavior;
- Linux CI on Ubuntu 22.04, Ubuntu 24.04, and `ubuntu-latest`;
- Windows MinGW CI on `windows-latest`;
- encoder, decoder, and validator integration tests on Linux and Windows;
- installed-package exact-version external consumer tests;
- Cortex-M7 compile and relocatable-link validation against `libccsdspack.a`.

### Native hardware evidence

| Target | Date | Result | Evidence boundary |
|---|---|---|---|
| Raspberry Pi 5, native arm64 Debian 13.5 | 2026-07-26 | PASS | 93/93 installed tests, CLI integration, exact-version external consumer, `CCSDSPACK_AARCH64_TEST:PASS` |
| NUCLEO-H755ZI-Q, Cortex-M7 using the retained H745-labelled dual-core validation project | 2026-08-02 | PASS | firmware built, linked, flashed, started, and produced `CCSDSPACK_MCU_TEST:PASS` twice over ST-LINK virtual COM at 115200 8N1 |

The STM32 UART banner says `STM32H745 CM7 hardware validation` because the retained project and harness use the H745 label. The validation procedure documents the accepted H745/H755 shared-project mapping used for the NUCLEO-H755ZI-Q run.

Hardware execution demonstrates successful operation on the tested targets. It does not expand the protocol claim beyond the Space Packet PDU profile or constitute qualification of every compiler, board revision, timing condition, memory layout, or mission configuration.

## Compliance evidence policy

A matrix row is classified as **Implemented** only when the repository contains:

1. a normative clause or documented derived/external requirement;
2. a concrete implementation reference;
3. focused regression evidence;
4. an independently generated expected value when wire behavior is involved;
5. a negative test when validation or rejection behavior is involved;
6. no contradictory supported configuration path.

Internal encode/decode round trips remain useful regression tests but are not treated as independent wire-format evidence by themselves.

## Release conclusion

All v1.2.0 Space Packet PDU implementation, automated validation, Raspberry Pi arm64 execution, and STM32 Cortex-M7 execution gates are complete.

Remaining release work is operational rather than protocol implementation:

1. merge the validated `develop` state into `main`;
2. confirm Linux and Windows CI at the selected `main` commit;
3. create tag `v1.2.0`;
4. publish and verify GitHub Release assets and GHCR images;
5. close release issue #95 and parent issue #46.
