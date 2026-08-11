<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v2 compliance statement

## Release claim

CCSDSPack v2.0.0 implements a documented **CCSDS 133.0-B-2, Issue 2, including Editorial Change 2, Space Packet PDU profile** together with supported direction-specific secondary-header layouts from **ECSS-E-70-41A (PUS-A)** and **ECSS-E-ST-70-41C (PUS-C)** and a documented subset of **CCSDS 301.0-B-4 basic CUC time**.

The PUS claim is limited to the implemented secondary-header codecs and documented tailoring. It is not a claim to implement complete PUS services or a complete PUS application.

## Covered scope

The release scope includes:

- the fixed six-octet Space Packet Primary Header and exact Packet Data Length semantics;
- Packet Version Number `000`, TM/TC Packet Types, the complete 11-bit APID range, Idle Packet structure, Sequence Flags, and modulo-16384 Packet Sequence Count handling;
- checked construction, finalization, serialization, bounded transactional parsing, stream management, and application-data reassembly;
- generic packet-level `PacketErrorControlMode::{CRC16,None}`;
- PUS-A and PUS-C TC/TM secondary headers represented by concrete revision/direction types;
- optional PUS tailoring for applicable identifier widths, PUS-A TM packet subcounter, spare octets, and TM CUC timestamps;
- PUS-C two-octet TC source ID and TM destination ID widths;
- numeric basic CUC time with explicit epoch metadata, P-field policy, and coarse/fine widths;
- fixed-capacity named `ValidationReport` checks for packet, template, sequence, secondary-header, PUS, and CUC state;
- vector and pointer-plus-size parsing/Manager interfaces;
- hosted and `CCSDS_MCU` C++17 library builds.

## Claim boundary

CCSDSPack v2.0.0 does not claim the complete abstract CCSDS Packet or Octet String Services, a complete protocol entity, a completed PICS, complete PUS services, transfer frames, virtual channels, CFDP, COP-1, transport bindings, calendar-time conversion, leap-second handling, or mission time correlation.

The optional CRC-16/CCITT-FALSE trailer is a CCSDSPack packet-level convention encoded inside the Packet Data Field; CCSDS 133.0-B-2 does not define it as a separate top-level Space Packet field. The optional Manager synchronization marker is external stream framing and is not part of a Space Packet.

## Validation and embedded profile

`ccsds::Validator` reports named `ValidationCode` checks without mutating the Packet or secondary header being inspected. `ValidationReport` stores performed checks in fixed `std::array` storage and performs no dynamic allocation itself.

`CCSDSPACK_BUILD_MCU=ON` builds the protocol library as a C++17 static archive and excludes host-only configuration and CLI components. MCU builds can disable exceptions and RTTI. The complete Packet/Manager implementation is not claimed to be heap-free.

## Evidence

The current integration candidate is covered by 125 native regression/conformance tests, independent fixed byte vectors, negative parser/PUS tests, Linux and Windows hosted CI, Doxygen, CLI integration, installed-package consumers and examples, Ubuntu 22.04 package/cross-build generation, and a Cortex-M compile/link probe exercising the public embedded API.

Fresh arm64 execution, physical STM32 execution, dedicated sanitizer/fuzz CI, and final release-publication checks remain release gates until recorded under the v2.0.0 milestone.

Detailed scope and traceability are maintained in:

- [CCSDS compliance matrix](CCSDS_COMPLIANCE.md);
- [Space Packet PDU profile](docs/CCSDS_133_0_B_2_PROFILE.md);
- [PUS tailoring](docs/MISSION_TAILORING.md);
- [Structured validation](docs/VALIDATION.md).
