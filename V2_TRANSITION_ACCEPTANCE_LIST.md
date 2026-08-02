# CCSDSPack v2.0.0 Transition Acceptance List

## Purpose

This document is the working acceptance baseline for CCSDSPack v2.0.0.

The implementation baseline is the released `v1.2.0` state on `main`, synchronized into `v2.0.0-dev` through issue #108. Generic CCSDS Space Packet work completed and validated in v1.2.0 is inherited by v2.0.0 and is not repeated merely because an older transition checklist predated that release.

Status markers:

- **Complete**: implemented and supported by merged evidence.
- **Inherited**: completed in v1.2.0 and carried into v2.0.0.
- **Pending**: required for v2.0.0.
- **Deferred**: explicitly outside the mandatory v2.0.0 scope.

## Release scope

CCSDSPack v2.0.0 targets:

- CCSDS 133.0-B-2 Issue 2, including Editorial Change 2, Space Packet PDU behaviour;
- ECSS-E-ST-70-41C PUS-C telecommand and telemetry secondary headers;
- explicit mission tailoring;
- deterministic packet finalization and strict profile validation;
- removal of the legacy project-specific `PusA`, `PusB`, and `PusC` public model;
- independent PUS-C conformance vectors and robustness testing.

The following remain outside v2.0.0:

- PUS-A compliance;
- complete implementation of every PUS service;
- CCSDS transfer frames, COP-1, CFDP, or transport bindings;
- a C core or stable C ABI;
- automatic reassembly of arbitrarily interleaved segmented streams.

---

# Phase 0: Standards and tailoring baseline

**Status: Complete**

Tracked by #43, #44, #45, and #91.

- [x] CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 is the Space Packet baseline.
- [x] ECSS-E-ST-70-41C is the sole PUS revision targeted by v2.0.0.
- [x] PUS-A is deferred.
- [x] The invalid PUS-B revision concept is rejected.
- [x] TC and TM directions are represented independently from PUS revision.
- [x] Mission-tailored identifier widths, packet error control, and telemetry time fields are documented.
- [x] `docs/CCSDS_COMPLIANCE.md` and `docs/MISSION_TAILORING.md` exist.
- [x] `inc/CCSDSMissionProfile.h` provides the initial public contract.

---

# Phase 1: Synchronize the v1.2.0 implementation baseline

**Status: In progress through #108**

- [x] Create `feature/sync-v1.2-baseline` from the released `main` state.
- [x] Preserve the completed Phase 0 v2 documents and public profile contract.
- [x] Resolve the duplicate `PacketErrorControlMode` declaration by reusing the v1.2 packet type.
- [x] Rebaseline this acceptance list and the transition roadmap.
- [ ] Merge the synchronization PR into `v2.0.0-dev`.
- [ ] Confirm Linux, Windows, Doxygen, CLI, installed-consumer, packaging, and MCU cross-build workflows on the synchronized branch.

---

# Phase 2: Generic CCSDS Space Packet foundation

**Status: Inherited from v1.2.0**

The following work is complete in the released baseline through #46 and its completed work items, including #47, #48, #49, #50, #51, #52, #53, #54, #55, #70, #73, #92, #93, #94, and #95.

## Packet encoding and parsing

- [x] Packet Data Length is encoded as Packet Data Field octets minus one.
- [x] CRC octets are included in Packet Data Length when enabled.
- [x] Complete packet-size boundaries are validated without integer wraparound.
- [x] Parsing validates the six-octet primary header before allocating packet data.
- [x] Parsing consumes exactly one declared packet.
- [x] Truncated packet bodies fail deterministically.
- [x] Concatenated packets can be parsed one at a time.
- [x] Trailing bytes remain unconsumed and consumed-byte count is reported.
- [x] Failed parsing does not partially mutate the destination packet.

## Packet error control

- [x] `None` and `CRC16` modes are supported.
- [x] CRC presence is explicitly configured and never inferred from trailing bytes.
- [x] CRC covers primary header, secondary header, and application data, excluding the CRC itself.
- [x] CRC is serialized big-endian and validated during parsing.
- [x] CRC corruption returns a dedicated checksum error.

## Header and sequence semantics

- [x] APID uses an 11-bit-capable type and accepts normal APIDs 0 through 2046.
- [x] APID 2047 is handled as the Idle Packet APID.
- [x] Values above 2047 are rejected rather than masked.
- [x] Invalid packet version, packet type, header flag, sequence flag, and sequence count values are rejected.
- [x] Unsegmented packets preserve non-zero sequence counts.
- [x] Automatic sequence counts advance modulo 16384.
- [x] Manual sequence-count mode is supported.
- [x] Read-only getters do not recalculate length, CRC, sequence count, or secondary-header state.

## Manager model

- [x] One `CCSDS::Manager` represents one complete Packet Identification stream.
- [x] One Manager owns one independent sequence counter for that stream.
- [x] Applications handling multiple APIDs use multiple Manager instances or direct Packet objects.
- [x] Mixed Packet Identification values are rejected transactionally.

The old requirement for one Manager to maintain an internal counter map for several APIDs is removed. It conflicts with the accepted single-stream Manager architecture and is not required to provide independent counters between APIDs.

## Generic evidence

- [x] Independent generic CCSDS golden vectors exist.
- [x] Generic malformed-packet and regression tests exist.
- [x] Linux and Windows tests run the vectors.
- [x] Installed CMake consumer tests exist.
- [x] CLI integration tests exist.
- [x] Native arm64 package validation and Cortex-M7 deterministic validation are recorded for v1.2.0.

---

# Phase 3: Mission-profile and typed secondary-header architecture

**Status: Pending**

Tracked by #57, #58, #65, #66, #67, #68, and #69.

## Mission profile

- [ ] A generic CCSDS packet profile can exist without enabling PUS.
- [ ] PUS-C requires explicit revision and direction selection.
- [ ] Source-ID and destination-ID widths are validated.
- [ ] Packet error control uses the common packet-layer enum.
- [ ] Supported telemetry time configuration is deterministic.
- [ ] Unsupported combinations return specific profile errors.
- [ ] Header sizes are derived from the validated profile.
- [ ] Profile validation is non-mutating and fully unit tested.

## Typed PUS architecture

- [ ] TC and TM are distinct concrete types.
- [ ] A TC secondary header cannot be attached to a TM packet.
- [ ] A TM secondary header cannot be attached to a TC packet.
- [ ] Factory selection uses structured identifiers rather than ambiguous strings.
- [ ] Factory creation returns a fresh mutable header instance.
- [ ] Custom and opaque secondary headers remain extensible.

## Packet lifecycle

- [ ] `finalize()` or an equivalent error-returning operation exists.
- [ ] Finalization validates profile and packet consistency.
- [ ] Finalization updates dependent secondary-header fields.
- [ ] Finalization calculates Packet Data Length and packet error control.
- [ ] Serialization exposes finalization failures instead of only returning an empty buffer.
- [x] Getters remain non-mutating, inherited from #70.

---

# Phase 4: Standards-oriented PUS-C implementation

**Status: Pending**

Tracked by #59, #60, and #61.

## PUS-C telecommand

- [ ] First-octet layout and reserved bit are correct.
- [ ] All acknowledgement flags are independently represented.
- [ ] Service type and subtype are encoded and decoded correctly.
- [ ] Source-ID width is profile-driven.
- [ ] No custom application-data-length field exists.
- [ ] Direction mismatch and reserved-bit failures are rejected.

## CCSDS time support

- [ ] The initial supported time-code family is explicitly selected.
- [ ] Coarse and fine lengths are represented separately where required.
- [ ] Epoch and P-field policy are documented.
- [ ] Encoded time size is deterministic.
- [ ] Invalid time configurations return specific errors.

## PUS-C telemetry

- [ ] First-octet layout and reserved bit are correct.
- [ ] Time-reference status is represented.
- [ ] Message-type counter is represented.
- [ ] Destination-ID width is profile-driven.
- [ ] Optional timestamp is profile-driven.
- [ ] No custom application-data-length field exists.
- [ ] Direction mismatch, malformed timestamp, and reserved-bit failures are rejected.

---

# Phase 5: Remove the legacy PUS model

**Status: Pending**

Tracked by #64 and #109.

- [ ] Remove legacy `PusB` completely; do not retain it as a standards-facing type.
- [ ] Remove legacy project-specific `PusA` and `PusC` classes.
- [ ] Remove `PusServices.h` and `PusServices.cpp` after all legacy types are retired.
- [ ] Remove automatic DataField registration of legacy PUS classes.
- [ ] Remove legacy PUS classes from `CCSDSPack.h`.
- [ ] Reject legacy configuration with a migration error.
- [ ] Remove legacy PUS use from current tests, examples, and diagrams.
- [ ] Preserve v1.2 historical documents as explicitly versioned evidence.
- [ ] Document replacements in `docs/MIGRATION_V1_TO_V2.md`.

## Deferred PUS-A work

Issues #62, #63, and #75 are deferred from v2.0.0 and should be closed as not planned for this release. Proper PUS-A may be reconsidered after v2.0.0 only when an interoperability requirement exists.

---

# Phase 6: Validation and conformance evidence

**Status: Partially inherited; PUS work pending**

Tracked by #71, #72, #73, #74, #76, and #77.

- [x] Generic CCSDS independent vectors exist through #73.
- [x] Generic parser, CRC, boundary, APID, sequence, and non-mutation regressions exist through #92.
- [ ] Validator reports structured profile and PUS-specific failures.
- [ ] Independent PUS-C TC vectors exist.
- [ ] Independent PUS-C TM vectors exist with and without timestamp.
- [ ] Identifier-width and reserved-bit vectors exist.
- [ ] Wrong-direction, invalid-profile, and malformed-time negative vectors exist.
- [ ] AddressSanitizer and UndefinedBehaviorSanitizer jobs pass.
- [ ] Primary-header, packet, and PUS parser fuzz targets exist.
- [ ] CI runs bounded fuzz smoke tests.

---

# Phase 7: Public API, configuration, tools, and documentation

**Status: Generic v1.2 capability inherited; v2 profile migration pending**

Tracked by #78 through #87.

## Configuration and APIs

- [ ] New configuration explicitly selects generic or PUS profile.
- [ ] PUS revision and packet direction are explicit.
- [ ] Identifier widths and time settings are explicit.
- [ ] Legacy `secondary_header_type=PusA|PusB|PusC` values fail clearly.
- [ ] Public headers expose no invalid PUS concept.
- [ ] Installed consumer compiles generic, PUS-C TC, and PUS-C TM examples.

## Manager and command-line tools

- [x] Generic Manager sequence and segmentation behaviour is inherited from v1.2.0.
- [x] Generic encoder Packet Data Length and packet-error-control behaviour is inherited from #93.
- [x] Generic decoder bounded-stream behaviour is inherited from #93.
- [x] Generic validator and CLI exit behaviour is inherited from #93.
- [ ] Manager generation and parsing use the validated mission profile.
- [ ] Encoder accepts v2 profile configuration and matches PUS-C vectors.
- [ ] Decoder reports PUS revision, direction, and fields.
- [ ] Validator reports profile and PUS failures through the library validator.

## Documentation and packaging

- [ ] Current README describes PUS-C scope and PUS-A deferral.
- [ ] Current examples and diagrams contain no legacy PUS model.
- [ ] `docs/MIGRATION_V1_TO_V2.md` covers every removed or replaced API.
- [ ] Linux, Windows, Doxygen, CLI, installed-consumer, package, arm64, and MCU jobs pass for v2.
- [ ] Release workflow uses v2 release notes and version-neutral artifact naming.

---

# Phase 8: Release preparation

**Status: Pending**

Tracked by #88.

- [ ] Project version is `2.0.0`.
- [ ] Shared-library SOVERSION reflects the breaking release.
- [ ] Release notes enumerate API, configuration, and wire-contract changes from v1.2.0.
- [ ] Compliance matrix reflects the final implementation and evidence.
- [ ] Every milestone issue is complete or explicitly deferred.
- [ ] Linux, Windows, sanitizer, fuzz, CLI, installed-consumer, package, arm64, and MCU gates pass.
- [ ] `v2.0.0-dev` is merged into `develop`.
- [ ] `develop` is merged into `main`.
- [ ] Tag `v2.0.0` is created from the approved `main` commit only.

---

# Release gate

CCSDSPack v2.0.0 may be released only when:

- [ ] the v1.2.0 baseline synchronization is complete;
- [ ] proper PUS-C TC and TM codecs pass independent vectors;
- [ ] the mission profile and finalization paths are validated and deterministic;
- [ ] legacy `PusA`, `PusB`, and `PusC` classes are absent from current public/runtime code;
- [ ] generic and PUS negative vectors pass;
- [ ] Linux, Windows, sanitizer, fuzz, CLI, installed-consumer, package, arm64, and MCU gates pass;
- [ ] README, migration guidance, compliance claims, and release notes match the actual implementation;
- [ ] deferred PUS-A work is excluded from the release claim.
