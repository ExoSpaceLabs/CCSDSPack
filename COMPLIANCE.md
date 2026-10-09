<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v2.1 compliance statement

## Release claim

CCSDSPack v2.1.0 preserves the documented **CCSDS 133.0-B-2, Issue 2,
including Editorial Change 2, Space Packet PDU profile** together with the
supported direction-specific secondary-header layouts from
**ECSS-E-70-41A (PUS-A)** and **ECSS-E-ST-70-41C (PUS-C)** and the documented
subset of **CCSDS 301.0-B-4 basic CUC time**.

The v2.1 C11-core migration changes implementation architecture, ownership,
copying, and allocation behavior. It does **not** expand or relax the normative
claim. The C and C++ layers must produce the same standards-defined wire
encodings and preserve the same accepted/rejected semantic states.

The PUS claim remains limited to the implemented TC/TM secondary-header codecs
and documented tailoring. It is not a claim to implement complete PUS services
or a complete PUS application.

## Covered scope

The release scope includes:

- the fixed six-octet Space Packet Primary Header and exact Packet Data Length semantics;
- Packet Version Number `000`, TM/TC Packet Types, the complete 11-bit APID range,
  Idle Packet structure, Sequence Flags, and modulo-16384 Packet Sequence Count handling;
- checked construction, finalization, serialization, bounded transactional parsing,
  stream management, and application-data reassembly;
- generic packet-level `PacketErrorControlMode::{CRC16,None}`;
- PUS-A and PUS-C TC/TM secondary headers with intrinsic revision/direction identity;
- supported direction-specific PUS tailoring, reserved/spare-field validation,
  identifier-width constraints, PUS-A TM packet subcounter, and optional TM CUC timestamps;
- PUS-C two-octet TC source ID and TM destination ID;
- numeric basic CUC time with explicit epoch metadata, P-field policy,
  and coarse/fine widths;
- fixed-capacity named validation checks for packet, template, sequence,
  secondary-header, PUS, and CUC state;
- native C11 caller-buffer/view APIs and the compatible C++17 Packet/Manager ownership layer;
- hosted and `CCSDSPACK_BUILD_MCU` builds.

## Claim boundary

CCSDSPack v2.1.0 does not claim the complete abstract CCSDS Packet or Octet
String Services, a complete protocol entity, a completed PICS, complete PUS
services, transfer frames, virtual channels, CFDP, COP-1, transport bindings,
calendar-time conversion, leap-second handling, or mission time correlation.

The optional CRC-16/CCITT-FALSE trailer is a CCSDSPack packet-level convention
encoded inside the Packet Data Field; CCSDS 133.0-B-2 does not define it as a
separate top-level Space Packet field. The optional Manager synchronization
marker is external stream framing and is not part of a Space Packet.

## Conformance preservation across the C-core migration

Performance or implementation changes are accepted only when the existing
standards evidence remains green:

- independent fixed generic Space Packet vectors;
- PUS-A TC/TM fixed vectors and tailoring/negative fixtures;
- PUS-C TC/TM fixed vectors and reserved/direction/tailoring negatives;
- the complete independent PUS-C acknowledgement matrix `0x0..0xF`,
  traced to ECSS-E-ST-70-41C clause 7.4.4.1;
- structured negative validation coverage for all 26 public `ValidationCode` entries;
- bounded parser/length/CRC/version/identifier/segmentation negative tests;
- sanitizer and bounded fuzz robustness gates.

Round-trip behavior alone is not treated as sufficient conformance evidence.

## v2.1 release evidence

The v2.1.0 release is covered by:

- **134/134 native regression/conformance tests**;
- Linux and Windows hosted CI, Doxygen, CLI integration, installed-package
  consumers/examples, and native/cross-package generation;
- dedicated Clang ASan and UBSan execution plus bounded libFuzzer smoke tests;
- a true C-only build/consumer path with no C++ compiler/runtime dependency;
- matched v2.0-vs-v2.1 performance/allocation evidence for owning raw parsing
  and packet serialization;
- Raspberry Pi 5 native arm64 installed-package execution with
  `CCSDSPACK_HARDWARE_TEST:PASS` and `CCSDSPACK_AARCH64_TEST:PASS`;
- physical NUCLEO-H755ZI-Q / Cortex-M7 execution with
  `CCSDSPACK_HARDWARE_TEST:PASS` using the standalone DAS/OpenOCD harness.

Broader matched PUS/CUC/Validator/Manager/stream performance characterization
was completed in issue #163. The measured CUC wrapper regression remains tracked
in issue #166 as a focused optimization follow-up. Neither performance work item
changes the CCSDS/ECSS conformance boundary.

Detailed scope and traceability are maintained in:

- [CCSDS Space Packet compliance matrix](CCSDS_COMPLIANCE.md);
- [Space Packet PDU profile](docs/CCSDS_133_0_B_2_PROFILE.md);
- [PUS/CUC compliance baseline](docs/PUS_CUC_COMPLIANCE.md);
- [PUS tailoring](docs/MISSION_TAILORING.md);
- [PUS-C independent evidence](docs/PUS_C_EVIDENCE.md);
- [Structured validation](docs/VALIDATION.md);
- [Structured validation evidence](docs/VALIDATION_EVIDENCE.md);
- [Robustness validation](docs/ROBUSTNESS.md);
- [Performance evidence](docs/PERFORMANCE.md);
- [v2 hardware validation](docs/V2_HARDWARE_VALIDATION.md);
- [v2.1 release acceptance list](V2_TRANSITION_ACCEPTANCE_LIST.md).
