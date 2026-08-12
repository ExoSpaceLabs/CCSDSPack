<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v2 CCSDS Space Packet compliance matrix

## Status

This document records the release-facing CCSDS Space Packet traceability for v2.0.0. It is read together with `COMPLIANCE.md`, `docs/CCSDS_133_0_B_2_PROFILE.md`, `docs/MISSION_TAILORING.md`, `docs/VALIDATION.md`, `docs/PUS_C_EVIDENCE.md`, `docs/VALIDATION_EVIDENCE.md`, and `docs/ROBUSTNESS.md`.

The presence of an API or an internal round trip is not treated as conformance evidence by itself. Supported areas are tied to focused semantic tests, independently fixed vectors, structured negative fixtures, or integration/robustness evidence.

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
| Six-octet primary header | Direct | Implemented | primary-header field/range tests, independent vectors, declared-size fuzz target |
| Packet Version Number `000` | Direct | Implemented | construction/configuration/parser negatives and structured `PacketVersion` fixture |
| TM/TC Packet Type | Direct | Implemented | primary-header tests and directional secondary-header consistency fixtures |
| 11-bit APID / Idle APID | Direct | Implemented | APID range, Idle positive/negative tests |
| Sequence Flags / 14-bit count | Direct | Implemented | segmentation, continuity, rollover tests |
| Packet Data Length | Direct | Implemented | independent vectors, min/max/overflow/bounded-parse tests, Packet fuzz target |
| Optional secondary header | Direct/mission-tailored | Implemented | opaque/custom/PUS header tests and explicit flag/object mismatch fixture |
| User data field | Direct | Implemented | application-data and bounded-parser tests |
| Packet assembly | Implemented subset | Implemented | Packet/Manager generation and segmentation tests |
| Packet extraction | Implemented subset | Implemented | transactional bounded parser, Manager load tests, raw Packet fuzz target |
| Lower-layer transfer/reception | Outside library | Not applicable | documented scope boundary |
| Maximum packet size | Direct | Implemented | 65,542-octet boundary tests |
| PUS-A TC/TM headers | Supported ECSS subset | Implemented | fixed vectors, round trips, tailoring/negative tests, typed PUS fuzz path |
| PUS-C TC/TM headers | Supported ECSS subset | Implemented | fixed vectors, full `0x0..0xF` TC acknowledgement matrix, round trips, reserved/direction/tailoring negatives, typed PUS fuzz path |
| Basic numeric CUC | Supported CCSDS subset | Implemented | explicit/implicit P-field vectors, width/overflow negatives, CUC fuzz target |
| Structured validation | Library invariant | Implemented | all 26 public `ValidationCode` values traced in `docs/VALIDATION_EVIDENCE.md` |
| Memory/UB robustness | Release hardening | Implemented | dedicated Clang ASan and UBSan regression jobs plus bounded libFuzzer smoke CI |

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

## PUS-C independent acknowledgement evidence

ECSS-E-ST-70-41C clause 7.4.4.1, Figure 7-9 defines the PUS-C TC secondary-header first octet as a four-bit PUS version number followed by four acknowledgement flags. Requirement 7.4.4.1.c sets the PUS version to 2. Requirement 7.4.4.1.d assigns bits 3..0 to successful acceptance, start, progress, and completion report requests respectively.

`test/src/testGroupEvidence.cpp` therefore checks a literal independent matrix for every valid four-bit acknowledgement value `0x0` through `0xF`. With fixed service type `0x11`, subtype `0x01`, and source ID `0x1234`, the expected secondary-header vectors are `20 11 01 12 34` through `2F 11 01 12 34`. Encoder output is compared byte-for-byte with each literal vector, then each literal vector is independently decoded and inspected.

The complete derivation and vector table are recorded in `docs/PUS_C_EVIDENCE.md`.

## PUS and CUC invariants

Concrete PUS types provide immutable revision/direction identity. Direction-specific tailoring controls only optional wire-layout choices. PUS-C TC source ID and TM destination ID are fixed at two octets. Spare octets are required to be zero. PUS-C TM time-reference status is a four-bit value. Numeric CUC widths and P-field state are validated before commit.

## Structured validation evidence

The Validator reports named checks for applicable generic Packet, template, sequence, secondary-header, PUS, and CUC state. Template comparison covers Packet Identification, segmentation class, packet-level PEC, and secondary-header type/tailoring. Validation does not mutate the inspected Packet or secondary header.

The release matrix in `docs/VALIDATION_EVIDENCE.md` traces every public `ValidationCode` to a direct malformed fixture, an applicable template/sequence failure, or a deliberately positive-only classification check. The evidence suite also enumerates all 26 codes, verifies stable symbolic names, and verifies that `ValidationReport::Capacity` can hold the complete public set.

`PusHeader` is intentionally a positive classification entry emitted only after an installed secondary header identifies itself as PUS; inventing an impossible negative object solely to make that entry fail would not provide meaningful protocol evidence.

## Robustness evidence

`.github/workflows/robustness.yml` adds three required automated robustness paths on supported development/release branches:

- complete native regression/conformance execution under Clang AddressSanitizer;
- complete native regression/conformance execution under Clang UndefinedBehaviorSanitizer;
- bounded libFuzzer smoke execution with ASan+UBSan across declared-size/primary-header, generic Packet, typed PUS-A/PUS-C, and CUC targets.

Each fuzz target runs with bounded generated-input count, input length, per-input timeout, and RSS. Successful bounded Packet parsing is asserted never to consume beyond supplied input and to agree with the primary-header declared packet boundary. Successful CUC decode is asserted to re-encode to the same complete encoded value.

The object model remains vector-backed. These gates provide crash, over-read, undefined-behavior, timeout, and bounded-resource evidence; they are not a claim that v2.0.0 parsing is zero-copy or globally allocation-free. See `docs/ROBUSTNESS.md`.

## Current evidence

The release candidate is supported by:

- **132/132 native regression/conformance tests**;
- independent fixed generic and PUS byte vectors, including the complete 16-value PUS-C TC acknowledgement matrix;
- a documented negative matrix covering the complete 26-code structured Validator surface;
- negative tests for version, ranges, length, CRC, identifier, segmentation, sequence, PUS fields/tailoring, and CUC configuration;
- Linux and Windows CI plus Doxygen;
- CLI integration;
- installed-package consumer and standalone examples;
- Ubuntu 22.04 package/cross-build generation;
- Cortex-M compile/link coverage of Packet, PUS, raw-buffer, and Validator APIs;
- dedicated Clang ASan and UBSan native-suite jobs;
- bounded four-target libFuzzer smoke CI under ASan+UBSan.

Fresh native arm64 execution, fresh physical STM32 execution, and final release-publication checks remain release gates and will be added to this evidence set before tagging.

## Change control

Changes to packet wire behavior, selected normative baselines, or the conformance boundary require coordinated updates to this matrix, `COMPLIANCE.md`, the detailed PDU profile, PUS evidence, structured-validation evidence, and robustness documentation.
