# CCSDSPack v2.0.0 transition roadmap

## Objective

Deliver a breaking, standards-oriented v2 release on the validated v1.2 CCSDS 133.0-B-2 Space Packet foundation, with proper PUS-A and PUS-C TC/TM secondary headers and no legacy pseudo-PUS formats.

## Completed foundation

- v1.2 packet encoding, bounded parsing, CRC profile, APID, sequence, Manager, CLI, packaging, and hardware evidence were synchronized into `v2.0.0-dev`.
- CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 remains the generic packet baseline.
- ECSS-E-70-41A and ECSS-E-ST-70-41C are the selected PUS-A and PUS-C secondary-header baselines.
- PUS-B is rejected because ECSS-E-70-41B was never issued.

## Stage 1: Typed PUS architecture — implemented

- `PusRevision` and `PacketDirection` are independent strong types.
- TC and TM use distinct concrete classes for each revision.
- `SecondaryHeaderFactory` handles custom, direction-neutral extensions.
- fixed `PusSecondaryHeaderFactory` owns the reserved canonical selectors.
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
- 101 native tests, including the four-selector PUS factory matrix and v2 configuration naming;
- PUS-A/PUS-C fixed byte vectors and negative vectors;
- ASan/UBSan validation;
- MCU compile with `-fno-exceptions -fno-rtti`;
- diff hygiene;
- Linux, Windows, and Doxygen workflows on `develop`;
- installed-package consumer validation with v2 version expectations.
- standalone generic, custom, PUS-C TC, and PUS-C TM `find_package` examples, built and executed as installed-package consumers on Linux and Windows.

Remaining release-level evidence:

- installed-package and CLI PUS-profile integration;
- bounded fuzz smoke jobs;
- arm64 and STM32 hardware repetition with representative PUS vectors.

## Stage 6: Higher-level integration

- extend the configuration schema to construct complete PUS mission profiles;
- expose PUS fields through encoder, decoder, and validator tools;
- thread mission profiles through Manager workflows where automatic PUS construction/parsing is required;
- add dedicated numeric CCSDS time conversion only when its epoch and P-field policy are selected.

## Stage 7: Release preparation

1. Continue reviewed v2 changes through pull requests from `v2.0.0-dev` to `develop`.
2. Resolve integration failures without weakening fixed vectors or profile checks.
3. Complete remaining CLI/profile/fuzz/hardware gates.
4. Synchronize release notes and final compliance traceability.
5. Merge approved `develop` into `main` and tag `v2.0.0` from `main` only.
