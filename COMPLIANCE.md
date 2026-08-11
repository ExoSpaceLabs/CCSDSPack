<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v2 compliance statement

## Release claim

CCSDSPack v2 implements a documented **CCSDS 133.0-B-2, Issue 2, including
Editorial Change 2, Space Packet PDU profile**. It also implements the supported
direction-specific secondary-header layouts from **ECSS-E-70-41A (PUS-A)** and
**ECSS-E-ST-70-41C (PUS-C)**.

The PUS claim is limited to the secondary-header codecs and their documented
mission tailoring. It is not a claim to implement every PUS service or a
complete PUS application.

## Covered scope

- fixed six-octet Space Packet Primary Header and exact Packet Data Length;
- Packet Version Number `000`, TM/TC Packet Types, the 11-bit APID range, Idle
  Packet structure, Sequence Flags, and modulo-16384 sequence count;
- checked construction, serialization, bounded parsing, stream management, and
  structured validation;
- explicit PUS-A/PUS-C revision and TC/TM selection;
- service, subtype, acknowledgement, source/destination, counter,
  four-bit time-reference-status, optional PUS-A subcounter, and spare fields
  applicable to the selected layout;
- numeric basic CCSDS CUC time with explicit epoch, P-field policy, and
  coarse/fine widths;
- fixed-capacity named `ValidationReport` checks for generic CCSDS, templates,
  mission profiles, and the supported PUS fields;
- independent vectors, negative tests, CLI round trips, and installed-package
  consumer tests.

## Claim boundary

CCSDSPack v2 does not claim the complete abstract CCSDS Packet or Octet String
Services, a complete protocol entity, a completed PICS, all PUS services,
transfer frames, virtual channels, CFDP, COP-1, transport bindings, calendar time
conversion, leap-second handling, or mission time correlation.

The optional CRC-16/CCITT-FALSE trailer is a CCSDSPack mission-profile convention
inside the Packet Data Field. CCSDS 133.0-B-2 does not define it as a separate
top-level field. The optional Manager synchronization marker is external stream
framing, not part of a Space Packet.

## Validation and bare-metal profile

`ccsds::Validator` reports named `ValidationCode` checks and does not mutate the
Packet, MissionProfile, or secondary header being inspected. Its
`ValidationReport` stores checks in fixed `std::array` storage and performs no
dynamic allocation itself.

The protocol library remains C++17 and the Validator is part of the
`CCSDS_MCU` static-library build. MCU builds can disable exceptions and RTTI;
host-only Config and command-line components are excluded.

## Evidence

Technical scope and traceability are documented in:

- [CCSDS compliance matrix](CCSDS_COMPLIANCE.md);
- [Space Packet PDU profile](docs/CCSDS_133_0_B_2_PROFILE.md);
- [PUS and mission tailoring](docs/MISSION_TAILORING.md);
- [structured validation](docs/VALIDATION.md);
- [configuration reference](docs/CONFIG.md);
- independent vectors and regression tests under `test/`;
- Linux and Windows CI, CLI integration, examples, sanitizers, and package
  consumer builds.

Historical v1.2 execution on Raspberry Pi 5 and NUCLEO-H755ZI-Q supports the
inherited packet core only. It does not replace v2 hardware revalidation for the
new namespace, PUS, time, Validator, or configuration APIs.

Automatic UML generation is currently disabled and is not part of the v2.0.0
compliance or release gate. The workflow remains available manually for later
documentation maintenance.
