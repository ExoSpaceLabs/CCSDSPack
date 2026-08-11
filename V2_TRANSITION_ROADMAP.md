# CCSDSPack v2.0.0 transition roadmap

## Objective

Deliver a breaking, standards-oriented C++17 v2 release on the validated v1.2 CCSDS 133.0-B-2 Space Packet foundation, with proper PUS-A and PUS-C TC/TM secondary headers, structured validation, explicit mission tailoring, and no legacy pseudo-PUS formats.

The public protocol library must remain usable in hosted and bare-metal builds. The MCU profile therefore remains compatible with `-fno-exceptions -fno-rtti`; host-only configuration and CLI code stay outside `CCSDS_MCU`.

## Completed foundation

- v1.2 packet encoding, bounded parsing, CRC profile, APID, sequence, Manager, CLI, packaging, and hardware evidence were synchronized into `v2.0.0-dev`.
- CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 remains the generic packet baseline.
- ECSS-E-70-41A and ECSS-E-ST-70-41C are the selected PUS-A and PUS-C secondary-header baselines.
- PUS-B is rejected because ECSS-E-70-41B was never issued.
- the project language standard remains C++17.

## Stage 1: Typed PUS architecture — implemented

- `ccsds::pus::Revision` and `ccsds::pus::Direction` are independent strong types;
- TC and TM use distinct concrete classes for each revision;
- `ccsds::SecondaryHeaderFactory` handles custom, direction-neutral extensions;
- fixed `ccsds::pus::SecondaryHeaderFactory` owns the reserved canonical selectors;
- both factory paths create fresh mutable instances;
- all four canonical selector paths are covered directly by the PUS factory tests.

## Stage 2: Mission profiles — implemented

- a default profile is generic CCSDS;
- PUS requires explicit revision and direction;
- identifier, error-control, timestamp, PUS-A subcounter, and spare choices are validated;
- header size is derived from the validated profile;
- packet attachment and parsing validate profile, direction, and error-control consistency;
- numeric basic CUC uses explicit epoch, P-field, coarse-width, and fine-width policy.

## Stage 3: PUS-A/PUS-C secondary headers — implemented

- PUS-A TC/TM version, reserved, acknowledgement, service, subtype, optional identifier, subcounter, timestamp, and spare fields;
- PUS-C TC acknowledgement/service/subtype/two-octet source fields;
- PUS-C TM four-bit time-reference/service/subtype/counter/two-octet destination/timestamp fields;
- canonical selectors `PUS:revA:TC`, `PUS:revA:TM`, `PUS:revC:TC`, and `PUS:revC:TM`.

## Stage 4: Remove legacy model — implemented

- removed the legacy `PusA`, `PusB`, and `PusC` public/runtime classes;
- removed `PusServices.h/.cpp` and automatic registration;
- renamed the public attachment, inspection, factory, and primary-header flag APIs to use CCSDS secondary-header terminology;
- removed current tests and fixtures for the legacy formats;
- legacy configuration selectors fail with migration guidance;
- documented the breaking API/wire migration.

## Stage 5: Structured validation — implementation complete, integration under review

The inherited positional six-boolean Validator model has been replaced by named checks:

- `ccsds::ValidationCode` identifies generic CCSDS, mission-profile, template, and PUS failures;
- `ccsds::ValidationReport` uses fixed `std::array` storage with bounded capacity;
- `ccsds::Validator::validate()` does not mutate the packet, profile, or secondary header;
- sequence validation retains one stateful modulo-16384 stream per Validator instance;
- PUS validation covers revision, direction, Packet Type, profile, header size, reserved/spare fields, TC acknowledgement/source ID, TM destination ID, PUS-A subcounter policy, PUS-C time-reference status, and CUC timestamp fit;
- `ccsds_validator` delegates protocol/profile checks to the library instead of maintaining parallel validation rules;
- the API remains C++17 and is compiled into `CCSDS_MCU` without requiring RTTI or exceptions;
- the ARM compile/link consumer probe exercises the structured report and a representative PUS-C TC packet.

The current hosted native suite contains 108 passing tests after the Validator additions.

## Stage 6: Evidence and hardening — ongoing

Completed or present:

- checked `Packet::update()`, `Packet::serialize()`, `DataField::serialize()`, and Manager stream serialization results;
- native tests covering finalization, profile, header, data-length, Manager, and structured Validator behavior;
- PUS-A/PUS-C fixed byte vectors and malformed vectors;
- local ASan/UBSan validation;
- MCU build design for `-fno-exceptions -fno-rtti`;
- Linux, Windows, and Doxygen hosted workflows;
- installed-package consumer validation with v2 version expectations;
- standalone generic, custom, PUS-C TC, and PUS-C TM `find_package` examples.

Remaining release-level evidence:

- final Linux package/cross-build validation with the new MCU Validator/PUS probe;
- committed sanitizer jobs and bounded parser fuzz smoke jobs;
- completion of the PUS-C acknowledgement-vector matrix and final traceability;
- final negative-vector coverage through the structured Validator;
- arm64 and STM32 v2 execution with representative PUS/Validator paths.

UML diagrams are deliberately **not** a v2.0.0 release gate. Automatic UML generation is disabled because its processing cost currently exceeds its release value. The workflow remains available manually through `workflow_dispatch` and can be re-enabled later.

## Stage 7: User-facing integration — substantially implemented

- the public namespace is `ccsds`, with revision-specific PUS codecs under `ccsds::pus::rev_a` and `ccsds::pus::rev_c`;
- configuration constructs generic and complete PUS-A/PUS-C TC/TM mission profiles and rejects legacy mappings;
- encoder, decoder, and validator use the configured PUS profile;
- Manager applies the template profile during automatic PUS parsing;
- numeric basic CUC supports CCSDS/agency epoch metadata, implicit/explicit P-field, and validated coarse/fine widths;
- CLI integration round-trips committed generic/PUS profiles and rejects malformed PUS input;
- the public documentation describes structured validation and hosted versus bare-metal ownership.

Remaining documentation work is concentrated in the complete before/after v1.2-to-v2 migration guide and final release notes/traceability.

## Stage 8: Release preparation

1. Complete reviewed feature branches into `v2.0.0-dev`.
2. Finish #74, #76, #77, #85, and the remaining #87 release gates without weakening fixed vectors or profile checks.
3. Record fresh arm64 and STM32 v2 evidence.
4. Approve release notes and final compliance traceability.
5. Merge approved `v2.0.0-dev` into `develop`.
6. Run the integrated `develop` gates.
7. Merge approved `develop` into `main`.
8. Run final `main` CI and tag `v2.0.0` from the approved `main` commit only.
