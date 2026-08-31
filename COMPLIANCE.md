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

The release evidence traces all 26 public validation codes to direct malformed fixtures, applicable template/sequence failures, or an explicitly documented positive-only classification check. PUS-C TC acknowledgement encoding is independently checked for all 16 four-bit combinations defined by ECSS-E-ST-70-41C clause 7.4.4.1.

`CCSDSPACK_BUILD_MCU=ON` builds the protocol library as a C++17 static archive and excludes host-only configuration and CLI components. MCU builds can disable exceptions and RTTI. The complete Packet/Manager implementation is not claimed to be heap-free.

## Robustness evidence

The release CI contains dedicated Clang AddressSanitizer and UndefinedBehaviorSanitizer jobs running the complete native regression/conformance suite. A separate bounded libFuzzer smoke gate exercises:

- primary-header / declared packet-size inspection;
- generic pointer-plus-size bounded Packet parsing;
- typed PUS-A and PUS-C TC/TM parsing with valid optional tailoring combinations;
- CUC configuration and decode/encode behavior.

The fuzz gate runs under ASan+UBSan with bounded input size, per-input timeout, and RSS. It is memory-safety and robustness evidence, not a claim of exhaustive input-space proof, zero-copy parsing, or globally allocation-free behavior.

## Evidence

The current release candidate is covered by **132 native regression/conformance tests**, independent fixed byte vectors, the complete PUS-C TC acknowledgement matrix, the complete structured-validation evidence matrix, Linux and Windows hosted CI, Doxygen, CLI integration, installed-package consumers and examples, Ubuntu 22.04 package/cross-build generation, a Cortex-M compile/link probe, dedicated ASan and UBSan regression jobs, and bounded four-target libFuzzer smoke CI.

Fresh native arm64 execution, physical STM32 execution, and final release-publication checks remain release gates until recorded under the v2.0.0 milestone.

Detailed scope and traceability are maintained in:

- [CCSDS compliance matrix](CCSDS_COMPLIANCE.md);
- [Space Packet PDU profile](docs/CCSDS_133_0_B_2_PROFILE.md);
- [PUS tailoring](docs/MISSION_TAILORING.md);
- [PUS-C independent evidence](docs/PUS_C_EVIDENCE.md);
- [Structured validation](docs/VALIDATION.md);
- [Structured validation evidence](docs/VALIDATION_EVIDENCE.md);
- [Robustness validation](docs/ROBUSTNESS.md).
