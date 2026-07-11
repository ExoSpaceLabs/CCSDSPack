# CCSDSPack v2.0.0 Transition Roadmap

## Project

**CCSDSPack 2.0.0 Compliance Assurance**

## Roadmap Objective

Deliver a standards-oriented v2.0.0 release by correcting the current C++ implementation before considering a future C-core migration.

The roadmap prioritizes wire compatibility, deterministic parsing, PUS-C correctness, explicit mission tailoring, and independent conformance evidence.

---

# Phase 0: Scope and Standards Baseline

## Goal

Establish the exact compliance target before modifying wire behaviour.

## Included Issues

- Define the v2.0.0 standards and compliance baseline.
- Create the CCSDS compliance matrix.
- Define supported PUS revisions.
- Define mission-tailoring requirements.

## Exit Criteria

- Exact standard revisions are selected.
- PUS-C is confirmed as the primary v2 target.
- PUS-A is either accepted or formally deferred.
- Unsupported layers are documented.
- Compliance claims have a traceable baseline.

## Deliverables

- `docs/CCSDS_COMPLIANCE.md`
- Initial PUS revision decision
- Initial mission-tailoring specification

---

# Phase 1: CCSDS Wire-Format Corrections

## Goal

Correct the core CCSDS packet bytes before redesigning higher-level PUS abstractions.

## Included Issues

1. Correct Packet Data Length encoding.
2. Enforce packet boundaries during deserialization.
3. Make packet error control optional.
4. Correct CRC calculation coverage.
5. Validate CRC during parsing.
6. Correct Packet Sequence Count behaviour.
7. Maintain independent sequence counters per APID.
8. Correct APID type and range validation.
9. Replace silent field masking with explicit validation.

## Exit Criteria

- Generic CCSDS packet encoding matches independent vectors.
- Parsing consumes exactly one packet.
- CRC and non-CRC packets are supported.
- Unsegmented packets may use non-zero sequence counts.
- APID values are not truncated.
- Invalid values return errors instead of being silently masked.

## Recommended PR Sequence

1. Primary-header validation and APID fixes.
2. Packet Data Length correction.
3. Packet-boundary parsing.
4. Optional packet error control.
5. CRC coverage and verification.
6. Sequence-count and APID counter behaviour.

---

# Phase 2: PUS Model Redesign

## Goal

Replace the current PUS-A/B/C class model with explicit PUS revision and packet direction.

## Included Issues

1. Introduce explicit PUS revision and packet direction types.
2. Separate PUS TC and TM abstractions.
3. Remove the invalid PusB secondary header.
4. Decide the migration path for the current PusB wire format.

## Exit Criteria

- PUS revision and TC/TM direction are independent concepts.
- Standards-facing APIs no longer expose `PusB`.
- A TC header cannot be attached to a TM packet.
- A TM header cannot be attached to a TC packet.
- Legacy PusB behaviour is either removed or clearly renamed as proprietary.

## Recommended PR Sequence

1. Add PUS revision and direction types.
2. Introduce TC/TM base abstractions.
3. Update factory selection.
4. Remove or rename PusB.

---

# Phase 3: Mission Profiles and PUS-C Implementation

## Goal

Implement standards-oriented PUS-C TC and TM headers using explicit mission tailoring.

## Included Issues

1. Add a mission profile model.
2. Add mission profile validation.
3. Refactor secondary-header factory selection.
4. Implement PUS-C TC secondary header.
5. Implement PUS-C TM secondary header.
6. Implement CCSDS time-field support for PUS-C TM.

## Exit Criteria

- Source-ID and destination-ID widths are profile-driven.
- Packet error control is profile-driven.
- PUS-C TC acknowledgement flags are supported.
- PUS-C TM time-reference status and message-type counter are supported.
- Optional timestamps use an explicitly configured CCSDS time format.
- PUS-C vectors pass independently verified tests.

## Recommended PR Sequence

1. Mission profile structure.
2. Mission profile validation.
3. Typed factory selection.
4. PUS-C TC implementation.
5. PUS-C TM implementation without timestamp.
6. CCSDS time-field implementation.
7. PUS-C TM timestamp integration.

---

# Phase 4: Deterministic Packet Lifecycle

## Goal

Remove hidden mutation and make packet preparation explicit.

## Included Issues

1. Introduce explicit packet finalization.
2. Remove hidden mutation from getters.
3. Update packet serialization to use finalized state.
4. Preserve parsed packet values during inspection.

## Exit Criteria

- Read-only getters are non-mutating.
- Parsed packets remain unchanged during inspection.
- Packet Data Length and CRC are generated during explicit finalization.
- Finalization reports validation errors before serialization.

## Recommended PR Sequence

1. Add explicit finalization API.
2. Move current `update()` responsibilities into finalization.
3. Make getters `const` where possible.
4. Remove implicit updates from accessors.

---

# Phase 5: Validation and Conformance Evidence

## Goal

Demonstrate standards compliance independently from internal round-trip behaviour.

## Included Issues

1. Extend the packet validator.
2. Add independent CCSDS golden vectors.
3. Add independent PUS-C golden vectors.
4. Add negative packet validation vectors.
5. Add sanitizer and fuzz testing.
6. Add PUS-A golden vectors only if PUS-A remains in scope.

## Exit Criteria

- CCSDS and PUS-C output matches independent byte vectors.
- Packet Data Length and CRC are independently verified.
- Invalid packets fail for specific reasons.
- Sanitizer jobs pass.
- Fuzz smoke tests run in CI.

## Recommended PR Sequence

1. CCSDS golden vectors.
2. CRC vectors.
3. PUS-C TC/TM vectors.
4. Validator expansion.
5. Negative vectors.
6. Sanitizer and fuzz CI.

---

# Phase 6: Public API, Tool, and Documentation Migration

## Goal

Migrate user-facing components to the compliant v2 model.

## Included Issues

1. Migrate packet configuration files.
2. Migrate `CCSDS::Manager`.
3. Migrate encoder CLI.
4. Migrate decoder CLI.
5. Migrate validator CLI.
6. Update examples and diagrams.
7. Add the v1-to-v2 migration guide.
8. Update README and compliance claims.
9. Update CI and package validation.

## Exit Criteria

- Configuration requires explicit PUS revision, direction, profile, and CRC mode.
- CLI tools generate and parse compliant packets.
- Existing incorrect configuration fails with a clear migration error.
- README claims match actual support.
- Installed package consumer tests pass.

## Recommended PR Sequence

1. Configuration migration.
2. Manager migration.
3. Encoder migration.
4. Decoder migration.
5. Validator migration.
6. Examples and diagrams.
7. Documentation and migration guide.
8. Packaging and installed-consumer tests.

---

# Phase 7: Optional PUS-A Delivery

## Goal

Implement PUS-A only if there is a concrete interoperability requirement.

## Included Issues

1. Implement PUS-A TC secondary header.
2. Implement PUS-A TM secondary header.
3. Add PUS-A golden vectors.
4. Add PUS-A examples and profile documentation.

## Decision Gate

PUS-A should remain outside the mandatory v2.0.0 path unless one of the following exists:

- a target mission using PUS-A,
- a required ground-segment interface,
- an existing packet archive requiring PUS-A decoding,
- a committed compatibility requirement.

## Exit Criteria

- PUS-A support is either complete and tested or explicitly deferred to v2.1.0.
- No partial PUS-A implementation is advertised as compliant.

---

# Phase 8: Release Preparation

## Goal

Prepare and release CCSDSPack v2.0.0 only after compliance gates pass.

## Included Issues

1. Set project version to `2.0.0`.
2. Update shared-library versioning.
3. Complete release notes.
4. Complete compliance matrix.
5. Confirm deferred scope.
6. Build release packages.
7. Tag `v2.0.0`.

## Release Gates

- Generic CCSDS golden vectors pass.
- PUS-C TC and TM golden vectors pass.
- Packet Data Length vectors pass.
- CRC and non-CRC packet vectors pass.
- Negative parser and validator tests pass.
- Linux and Windows builds pass.
- MCU cross-build passes.
- Sanitizer jobs pass.
- CLI integration tests pass.
- Installed package consumer tests pass.
- README and compliance matrix are complete.
- No standards-facing `PusB` API remains.

---

# Suggested Project Columns

Use the following project workflow:

1. **Backlog**
2. **Standards Review**
3. **Ready for Implementation**
4. **In Progress**
5. **In Review**
6. **Conformance Verification**
7. **Blocked**
8. **Done**

---

# Suggested Labels

- `v2.0.0`
- `compliance`
- `ccsds`
- `pus-c`
- `pus-a`
- `breaking-change`
- `wire-format`
- `parser`
- `validation`
- `testing`
- `documentation`
- `cli`
- `configuration`
- `blocked-by-standard-review`

---

# Critical Path

The critical path for v2.0.0 is:

1. Standards baseline
2. Packet Data Length correction
3. Packet-boundary parsing
4. Optional packet error control
5. CRC correction and validation
6. Header and APID validation
7. PUS revision and TC/TM redesign
8. Mission profile implementation
9. PUS-C TC implementation
10. PUS-C TM implementation
11. Explicit packet finalization
12. Conformance vectors
13. Tool and configuration migration
14. Release validation

PUS-A is not on the critical path unless explicitly promoted into v2.0.0 scope.

---

# Future Roadmap After v2.0.0

The following work should be considered after the compliant C++ wire model is stable:

## v2.1.x

- PUS-A support if deferred
- Additional CCSDS time formats
- Additional PUS service-specific payload helpers
- Improved segmented packet validation
- More interoperability vectors

## v3.0.0 Candidate

- Introduce an allocation-free C core
- Preserve the v2 wire model and test vectors
- Implement the C++ API as a wrapper over the C core
- Add a stable C ABI
- Retain identical packet output between C and C++ APIs

This sequencing prevents the language rewrite from obscuring protocol defects and gives the future C core a stable, independently verified behavioural target.
