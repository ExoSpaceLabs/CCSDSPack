# CCSDSPack v2.0.0 Transition Roadmap

## Objective

Deliver a standards-oriented CCSDSPack v2.0.0 by building proper ECSS-E-ST-70-41C PUS-C support on the released v1.2.0 CCSDS Space Packet foundation.

The v1.2.0 release already corrected and validated the generic packet layer. The v2 critical path therefore begins with branch synchronization and mission-profile architecture, not with repeating completed Packet Data Length, CRC, APID, parser, sequence, CLI, or package work.

## Current baseline

- `main` contains the released and validated v1.2.0 implementation.
- `v2.0.0-dev` contains the completed Phase 0 standards and tailoring artifacts.
- Issue #108 synchronizes both histories through `feature/sync-v1.2-baseline` and a PR into `v2.0.0-dev`.
- Issue #109 tracks removal of the legacy project-specific `PusA` and `PusC` classes.
- PUS-A implementation is deferred from v2.0.0.

---

# Stage 0: Synchronize and rebaseline

## Goal

Create one mergeable v2 development history containing the released v1.2.0 implementation and the accepted v2 Phase 0 documents.

## Work

- Merge `main` and the existing `v2.0.0-dev` history through #108.
- Preserve the compliance matrix, mission-tailoring specification, roadmap, acceptance list, and mission-profile header.
- Keep Linux, Windows, and Doxygen workflows active for `v2.0.0-dev`.
- Resolve public-type collisions.
- Reclassify completed generic CCSDS tasks as inherited.
- Correct stale issue descriptions and defer PUS-A tasks.

## Exit criteria

- The synchronization PR is mergeable and passes branch CI.
- `v2.0.0-dev` contains every v1.2.0 source, test, tool, package, and hardware-validation asset.
- Phase 0 artifacts remain present.
- Remaining work is accurately represented by issues and acceptance criteria.

---

# Stage 1: Mission-profile foundation

## Goal

Create a deterministic public profile model usable by generic CCSDS packets and PUS-C packets.

## Issues

- #65 Introduce Mission Profiles and Deterministic Packet Finalization
- #66 Add a mission profile model
- #67 Add mission profile validation

## Work

- Separate generic packet profile choices from PUS-specific choices.
- Reuse one packet-error-control enum across packet and profile APIs.
- Require explicit PUS revision and direction for PUS packets.
- Define supported identifier widths.
- Select the initial supported CCSDS time representation and encode its parameters explicitly.
- Return specific profile-validation errors.
- Derive secondary-header sizes from validated profiles.

## Exit criteria

- Generic packets do not implicitly become PUS packets.
- Invalid profiles cannot be serialized or strictly parsed.
- Every profile constraint has positive and negative tests.

---

# Stage 2: Typed secondary-header architecture

## Goal

Replace ambiguous string-selected PUS types with direction-safe, freshly instantiated codecs.

## Issues

- #57 Introduce explicit PUS revision and packet direction types
- #58 Separate PUS TC and TM secondary-header abstractions
- #68 Refactor secondary-header factory selection

## Work

- Create separate TC and TM types.
- Prevent attaching a TC header to a TM packet and vice versa.
- Replace ambiguous `PusA`, `PusB`, and `PusC` string selection in standards-facing paths.
- Make factory lookup return a fresh mutable object rather than a shared prototype.
- Preserve custom and opaque secondary-header extension points.

## Exit criteria

- PUS revision and packet direction are independent strong types.
- Wrong-direction construction and parsing fail before serialization succeeds.
- Custom headers remain usable without pretending to be PUS.

---

# Stage 3: PUS-C codecs and CCSDS time

## Goal

Implement the actual reason for the breaking release: compliant PUS-C TC and TM secondary headers.

## Issues

- #59 Implement the PUS-C TC secondary header
- #61 Implement CCSDS time-field support for PUS-C TM
- #60 Implement the PUS-C TM secondary header

## Recommended order

1. PUS-C TC without changing the generic packet core.
2. Initial CCSDS time-code implementation with independent vectors.
3. PUS-C TM without timestamp.
4. PUS-C TM timestamp integration.

## Exit criteria

- TC acknowledgement, service, subtype, source-ID, and reserved-bit semantics match ECSS-E-ST-70-41C.
- TM time-reference, service, subtype, message counter, destination-ID, and optional timestamp semantics match the selected profile.
- No legacy custom application-data-length field remains.
- Independent TC and TM byte vectors pass.

---

# Stage 4: Packet finalization, Manager, and validation integration

## Goal

Thread the validated profile through packet creation, parsing, management, and validation.

## Issues

- #69 Introduce explicit packet finalization
- #72 Extend the packet validator
- #80 Migrate CCSDS Manager to compliant sequence and packet handling

## Work

- Add an error-returning finalization path.
- Validate direction, secondary-header type, profile, Packet Data Length, and packet error control together.
- Preserve the accepted one-Manager/one-Packet-Identification/one-counter architecture.
- Use multiple Manager instances for multiple APIDs.
- Replace the fixed boolean validator report with structured failure information suitable for PUS and profile errors.

## Exit criteria

- Finalization failures are inspectable and deterministic.
- Parsed packets remain unchanged during inspection.
- Manager generation and parsing use the selected profile.
- Validator covers generic CCSDS and supported PUS-C requirements.

---

# Stage 5: Remove the legacy PUS model

## Goal

Ensure v2 exposes only valid standards concepts.

## Issues

- #64 Remove the invalid PusB secondary header
- #109 Remove legacy PusA and PusC secondary-header classes

## Work

- Delete legacy `PusA`, `PusB`, and `PusC` runtime/public classes.
- Delete `PusServices.h` and `PusServices.cpp` after migration.
- Remove automatic DataField registration and umbrella exports.
- Reject legacy configuration with migration errors.
- Remove current examples, tests, and diagrams that depend on legacy formats.
- Keep v1.2 release documents as versioned historical evidence.

## Exit criteria

- No current public header or runtime source exposes the legacy PUS model.
- Repository searches find legacy names only in versioned history or migration documentation.

---

# Stage 6: Independent evidence and robustness

## Goal

Prove PUS-C behaviour independently and harden parsers against malformed input.

## Issues

- #71 Strengthen Validation and Conformance Testing
- #74 Add independent PUS-C golden vectors
- #76 Add negative packet validation vectors
- #77 Add sanitizer and fuzz testing

## Inherited evidence

- Generic CCSDS vectors from #73.
- Generic negative and regression coverage from #92.
- Linux, Windows, CLI, installed-consumer, arm64, and MCU evidence from v1.2.0.

## New work

- Independent PUS-C TC vectors covering acknowledgement combinations and identifier widths.
- Independent PUS-C TM vectors with and without timestamp.
- Wrong-direction, reserved-bit, malformed-time, and invalid-profile vectors.
- ASan and UBSan jobs.
- Bounded fuzz targets for primary header, packet, and PUS parsing.

## Exit criteria

- Independent vectors match encoded bytes and decoded fields.
- Every malformed vector fails for the expected reason.
- Sanitizer and bounded fuzz smoke jobs pass in CI.

---

# Stage 7: Public API, configuration, tools, and documentation migration

## Goal

Make every user-facing path use the v2 profile and PUS-C model.

## Issues

- #78 Migrate Public APIs, Tools, Documentation, and Release Packaging
- #79 configuration
- #81 encoder
- #82 decoder
- #83 validator
- #84 examples and diagrams
- #85 migration guide
- #86 README and compliance claims
- #87 CI and package validation

## Work

- Define a v2 configuration schema for generic CCSDS, PUS-C TC, and PUS-C TM.
- Make old PUS configuration fail with a useful migration error.
- Extend encoder, decoder, and validator without discarding the correct v1.2 stream and CRC foundations.
- Move validator CLI protocol checks into the library validator.
- Compile representative examples in CI.
- Update current diagrams and documentation.
- Add `docs/MIGRATION_V1_TO_V2.md` with before/after API and configuration examples.
- Use version-neutral release workflow inputs until the v2 release notes exist.

## Exit criteria

- Installed consumers can use the complete v2 public API.
- CLI outputs match independent PUS-C vectors.
- README and compliance claims match actual support and limitations.
- Linux, Windows, Doxygen, package, arm64, and MCU paths pass.

---

# Stage 8: Release preparation

## Goal

Publish v2.0.0 only after all compliance and migration gates pass.

## Issue

- #88 Prepare the v2.0.0 release

## Work

- Set project version and shared-library SOVERSION to 2.
- Complete release notes and compliance traceability.
- Verify all deferred scope, especially PUS-A.
- Generate and test release packages and container images.
- Repeat arm64 and MCU validation with PUS-C vectors.
- Merge `v2.0.0-dev` into `develop`, then `develop` into `main`.
- Tag `v2.0.0` from the approved `main` commit.

## Release gates

- Generic CCSDS regression and independent vectors pass.
- PUS-C TC and TM independent vectors pass.
- Mission-profile and negative vectors pass.
- Linux, Windows, Doxygen, sanitizer, fuzz, CLI, installed-consumer, package, arm64, and MCU gates pass.
- Current public/runtime code exposes no legacy PUS class.
- README, migration guide, compliance matrix, and release notes are synchronized.

---

# Deferred work after v2.0.0

PUS-A issues #62, #63, and #75 are not part of the mandatory v2.0.0 path. They should be closed as not planned for this release and recreated or reopened only when a concrete interoperability requirement exists.

Potential later work:

- proper PUS-A support;
- additional CCSDS time families;
- additional PUS service-specific payload helpers;
- more interoperability vectors;
- future allocation-free C core and stable C ABI without changing the verified v2 wire model.

# Critical path

1. #108 synchronize v1.2.0 into `v2.0.0-dev`.
2. #66 and #67 complete the profile model and validation.
3. #57, #58, and #68 establish typed TC/TM factory architecture.
4. #59, #61, and #60 implement PUS-C TC, time, and TM.
5. #69, #72, and #80 integrate finalization, validation, and Manager.
6. #64 and #109 remove legacy PUS classes.
7. #74, #76, and #77 complete conformance and robustness evidence.
8. #79 through #87 migrate user-facing paths and CI.
9. #88 completes the release.
