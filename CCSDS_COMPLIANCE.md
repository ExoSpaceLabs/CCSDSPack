<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v2 CCSDS Space Packet compliance matrix

## Status

This document records the release-facing CCSDS Space Packet traceability for v2.0.0. It is read together with `COMPLIANCE.md`, `docs/CCSDS_133_0_B_2_PROFILE.md`, `docs/MISSION_TAILORING.md`, and `docs/VALIDATION.md`.

The presence of an API or an internal round trip is not treated as conformance evidence by itself; each supported area is tied to focused tests, fixed vectors, or integration evidence.

## Normative baseline

| Area | Baseline | Implemented scope |
|---|---|---|
| Space Packet Protocol | CCSDS 133.0-B-2 Issue 2, June 2020, including Editorial Change 2, September 2024 | Space Packet PDU construction, checked serialization, bounded parsing, structured validation, segmentation/sequence handling, and stream management |
| Packet Utilisation Standard | ECSS-E-70-41A and ECSS-E-ST-70-41C | Supported PUS-A/PUS-C TC/TM secondary-header layouts |
| CCSDS time code | CCSDS 301.0-B-4 | Supported basic numeric CUC subset |

## Claim boundary

The v2.0.0 claim covers the implemented Space Packet PDU and the library behavior required to create, serialize, parse, inspect, validate, segment, and manage those packets.

Complete abstract Packet/Octet String service primitives, lower-layer packet transfer, network routing, transfer frames, virtual channels, COP-1, CFDP, transport bindings, complete PUS services, and a completed system-level PICS remain outside scope.

The optional CCSDSPack CRC16 trailer is an external Packet-level convention encoded inside the Packet Data Field and is not defined by CCSDS 133.0-B-2 as a third top-level field.

## Traceability status

| Area | Classification | Status | Evidence |
|---|---|---|---|
| Six-octet primary header | Direct | Implemented | primary-header field/range tests and independent vectors |
| Packet Version Number `000` | Direct | Implemented | construction/configuration/parser negative tests |
| TM/TC Packet Type | Direct | Implemented | primary-header tests and directional secondary-header consistency tests |
| 11-bit APID / Idle APID | Direct | Implemented | APID range, Idle positive/negative tests |
| Sequence Flags / 14-bit count | Direct | Implemented | segmentation, continuity, rollover tests |
| Packet Data Length | Direct | Implemented | independent vectors, min/max/overflow/bounded-parse tests |
| Optional secondary header | Direct/mission-tailored | Implemented | opaque/custom/PUS header tests |
| User data field | Direct | Implemented | application-data and bounded-parser tests |
| Packet assembly | Implemented subset | Implemented | Packet/Manager generation and segmentation tests |
| Packet extraction | Implemented subset | Implemented | transactional bounded parser and Manager load tests |
| Lower-layer transfer/reception | Outside library | Not applicable | documented scope boundary |
| Maximum packet size | Direct | Implemented | 65,542-octet boundary tests |
| PUS-A TC/TM headers | Supported ECSS subset | Implemented | fixed vectors, round trips, tailoring/negative tests |
| PUS-C TC/TM headers | Supported ECSS subset | Implemented | fixed vectors, round trips, reserved/direction/tailoring negatives |
| Basic numeric CUC | Supported CCSDS subset | Implemented | explicit/implicit P-field vectors and width/overflow negatives |
| Structured validation | Library invariant | Implemented | named `ValidationCode` tests and CLI delegation |

## Core Space Packet invariants

- the primary header is exactly six octets;
- Packet Data Length equals the number of following octets minus one;
- the complete supported packet size is 7 through 65,542 octets;
- parsing derives one exact packet boundary from Packet Data Length;
- truncated input is rejected without partial state commit;
- consumed-byte reporting preserves following packets/trailing data;
- APID spans the complete 11-bit range and `0x7FF` is Idle;
- Packet Sequence Count is 14 bits and rolls over modulo 16384;
- Secondary Header Flag reflects installed secondary-header presence;
- a directional secondary header must agree with Packet Type.

## Packet error control

`PacketErrorControlMode::None` reserves no trailer. `PacketErrorControlMode::CRC16` reserves the final two Packet Data Field octets for CRC-16/CCITT-FALSE. The trailer contributes to Packet Data Length, is encoded most-significant byte first, and is excluded from its own CRC calculation.

CRC coverage includes the primary header, optional secondary header, and application data. Bounded parsing validates the selected mode transactionally.

## PUS and CUC invariants

Concrete PUS types provide immutable revision/direction identity. Direction-specific tailoring controls only optional wire-layout choices. PUS-C TC source ID and TM destination ID are fixed at two octets. Spare octets are required to be zero. PUS-C TM time-reference status is a four-bit value. Numeric CUC widths and P-field state are validated before commit.

## Structured validation

The Validator reports named checks for applicable generic Packet, template, sequence, secondary-header, PUS, and CUC state. Template comparison covers Packet Identification, segmentation class, packet-level PEC, and secondary-header type/tailoring. Validation does not mutate the inspected Packet or secondary header.

## Current evidence

The integration candidate is supported by:

- 125 native regression/conformance tests;
- independent fixed generic and PUS byte vectors;
- negative tests for version, ranges, length, CRC, identifier, segmentation, sequence, PUS fields/tailoring, and CUC configuration;
- Linux and Windows CI plus Doxygen;
- CLI integration;
- installed-package consumer and standalone examples;
- Ubuntu 22.04 package/cross-build generation;
- Cortex-M compile/link coverage of Packet, PUS, raw-buffer, and Validator APIs;
- local ASan and UBSan runs.

Dedicated sanitizer/fuzz CI, the final PUS-C acknowledgement vector matrix, final negative-vector approval, fresh native arm64 execution, and fresh physical STM32 execution remain release gates and will be added to this evidence set before tagging.

## Change control

Changes to packet wire behavior, selected normative baselines, or the conformance boundary require coordinated updates to this matrix, `COMPLIANCE.md`, the detailed PDU profile, and validation documentation.
