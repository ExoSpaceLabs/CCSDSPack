# CCSDSPack v2.0.0 transition roadmap

## Objective

Deliver a breaking, standards-oriented v2 release on the validated v1.2 CCSDS 133.0-B-2 Space Packet foundation, with proper PUS-A and PUS-C TC/TM secondary headers and no legacy pseudo-PUS formats.

## Completed foundation

- v1.2 packet encoding, bounded parsing, CRC profile, APID, sequence, Manager, CLI, packaging, and hardware evidence were synchronized into `v2.0.0-dev`.
- CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 remains the generic packet baseline.
- ECSS-E-70-41A and ECSS-E-ST-70-41C are the selected PUS-A and PUS-C secondary-header baselines.
- PUS-B is rejected because ECSS-E-70-41B was never issued.

## Stage 1: Typed PUS architecture — implemented

- `ccsds::pus::Revision` and `ccsds::pus::Direction` are independent strong types.
- TC and TM use distinct concrete classes for each revision.
- `ccsds::SecondaryHeaderFactory` handles custom, direction-neutral extensions.
- fixed `ccsds::pus::SecondaryHeaderFactory` owns the reserved canonical selectors.
- both factory paths create fresh mutable instances.
- all four canonical selector paths are covered directly by the PUS factory tests.

## Stage 2: Mission profiles — implemented for the codec boundary

- a default profile is generic CCSDS;
- PUS requires explicit revision and direction;
- identifier, error-control, timestamp, PUS-A subcounter, and spare choices are validated;
- header size is derived from the validated profile;
- packet attachment and parsing validate profile, direction, and error-control consistency.

## Stage 3: PUS-A/PUS-C secondary headers — implemented

- PUS-A TC/TM version, reserved, acknowledgement, service, subtype, optional identifier, subcounter, timestamp, and spare fields;
- PUS-C TC acknowledgement/service/subtype/source fields;
- PUS-C TM time-reference/service/subtype/counter/destination/timestamp fields;
- canonical selectors `PUS:revA:TC`, `PUS:revA:TM`, `PUS:revC:TC`, and `PUS:revC:TM`.

## Stage 4: Remove legacy model — implemented

- removed the legacy `PusA`, `PusB`, and `PusC` public/runtime classes;
- removed `PusServices.h/.cpp` and automatic registration;
- renamed the public attachment, inspection, factory, and primary-header flag APIs to use CCSDS secondary-header terminology;
- removed current tests and fixtures for the legacy formats;
- legacy configuration selectors fail with migration guidance;
- documented the breaking API/wire migration.

## Stage 5: Evidence — codec evidence implemented, release evidence ongoing

Completed and integrated:

- checked `Packet::update()`, `Packet::serialize()`, `DataField::serialize()`, and Manager stream serialization results;
- native tests covering exact finalization, profile, header, data-length, and Manager propagation errors;
- 106 native tests, including the four-selector PUS factory/configuration matrices
  and numeric CUC vectors;
- PUS-A/PUS-C fixed byte vectors and negative vectors;
- ASan/UBSan validation;
- MCU compile with `-fno-exceptions -fno-rtti`;
- diff hygiene;
- Linux, Windows, and Doxygen workflows on `develop`;
- installed-package consumer validation with v2 version expectations.
- standalone generic, custom, PUS-C TC, and PUS-C TM `find_package` examples, built and executed as installed-package consumers on Linux and Windows.

Remaining release-level evidence:

- bounded fuzz smoke jobs;
- arm64 and STM32 hardware repetition with representative PUS vectors.

## Stage 6: Higher-level integration — implemented

- the public namespace is `ccsds`, with revision-specific PUS codecs under
  `ccsds::pus::rev_a` and `ccsds::pus::rev_c`;
- the configuration schema constructs generic and complete PUS-A/PUS-C TC/TM
  mission profiles and rejects legacy mappings;
- encoder, decoder, and validator use and report the configured PUS profile;
- Manager applies the template profile during automatic PUS parsing;
- numeric basic CUC supports CCSDS/agency epoch metadata, implicit/explicit
  P-field, and validated coarse/fine widths;
- CLI integration round-trips every committed generic/PUS profile and rejects a
  corrupted PUS secondary header.

## Stage 7: Release preparation

1. Continue reviewed v2 changes through pull requests from `v2.0.0-dev` to `develop`.
2. Resolve integration failures without weakening fixed vectors or profile checks.
3. Complete remaining fuzz/hardware gates.
4. Synchronize release notes and final compliance traceability.
5. Merge approved `develop` into `main` and tag `v2.0.0` from `main` only.
