# CCSDSPack v2.0.0 Transition Acceptance List

## Project

**CCSDSPack 2.0.0 Compliance Assurance**

## Objective

Make the existing C++ implementation compliant with the selected CCSDS Space Packet and ECSS PUS standards while preserving the current overall library architecture.

The v2.0.0 release must correct wire-format behaviour, redesign PUS support, introduce explicit mission tailoring, strengthen validation, and provide independent conformance evidence.

## Out of Scope for v2.0.0

- C core rewrite
- Stable C ABI
- C++ wrapper over a C implementation
- CCSDS transfer frames
- COP-1
- CFDP
- SpaceWire transport
- Full implementation of every PUS service
- Automatic reassembly of arbitrarily interleaved segmented streams

---

# Main Issue 1: Define the v2.0.0 Standards and Compliance Baseline

## Description

Define the exact CCSDS and ECSS standards targeted by CCSDSPack v2.0.0.

This issue establishes the authoritative compliance scope for implementation, testing, documentation, and release claims. Requirements must be classified as mandatory, optional, mission-tailored, unsupported, or deferred.

## Acceptance Criteria

- [ ] Exact CCSDS Space Packet Protocol document and issue are identified.
- [ ] Exact PUS-A reference is identified if PUS-A remains in scope.
- [ ] ECSS-E-ST-70-41C is identified as the supported PUS-C baseline.
- [ ] Mandatory and mission-tailored fields are documented.
- [ ] Unsupported protocol layers and services are explicitly listed.
- [ ] A compliance traceability document is added to the repository.

## Sub-Issue 1.1: Create the CCSDS Space Packet compliance matrix

### Description

Create a clause-by-clause matrix mapping CCSDS Space Packet Protocol requirements to the CCSDSPack implementation.

Each requirement must be marked as implemented, partially implemented, mission-tailored, unsupported, or not applicable. Supported requirements should reference source code and tests.

### Acceptance Criteria

- [ ] `docs/CCSDS_COMPLIANCE.md` is added.
- [ ] Primary-header requirements are mapped.
- [ ] Packet Data Length requirements are mapped.
- [ ] Sequence-control requirements are mapped.
- [ ] Packet boundary and parsing requirements are mapped.
- [ ] Packet error control assumptions are documented.
- [ ] Each implemented item references at least one test.

## Sub-Issue 1.2: Define the supported PUS revisions

### Description

Document how CCSDSPack represents PUS-A and PUS-C.

Clarify that PUS-A and PUS-C refer to revisions of the Packet Utilisation Standard and are not generic secondary-header categories. Define whether PUS-A is included in v2.0.0 or deferred.

### Acceptance Criteria

- [ ] PUS-A scope is explicitly accepted or deferred.
- [ ] PUS-C is identified as the primary v2 target.
- [ ] TC and TM headers are documented separately.
- [ ] The invalid PUS-B concept is explicitly rejected.
- [ ] Supported PUS revisions are represented by an enum or profile field.

## Sub-Issue 1.3: Define mission-tailoring requirements

### Description

Identify all packet fields and behaviours requiring mission-specific configuration.

### Acceptance Criteria

- [ ] Source-ID width is configurable.
- [ ] Destination-ID width is configurable.
- [ ] Packet error control mode is configurable.
- [ ] Time-field presence is configurable.
- [ ] Time-code format and length are configurable.
- [ ] Invalid profile combinations are documented.

---

# Main Issue 2: Correct CCSDS Space Packet Wire Compliance

## Description

Correct CCSDS Space Packet serialization, deserialization, validation, packet-length handling, CRC behaviour, APID handling, and sequence-control semantics.

## Acceptance Criteria

- [ ] Packet Data Length is encoded correctly.
- [ ] Packet boundaries are enforced during parsing.
- [ ] Packet error control is optional.
- [ ] CRC coverage is correct.
- [ ] Sequence counts follow CCSDS semantics.
- [ ] APIDs and header fields are validated without silent truncation.
- [ ] Standards-compliant golden vectors pass.

## Sub-Issue 2.1: Correct Packet Data Length encoding

### Description

Update serialization so Packet Data Length contains the number of octets in the packet data field minus one.

The packet data field includes the secondary header, application data, and packet error control field when enabled.

### Acceptance Criteria

- [ ] Packet Data Length equals actual bytes following the primary header minus one.
- [ ] CRC bytes are included when packet error control is enabled.
- [ ] Maximum representable packet size is validated.
- [ ] Integer overflow is prevented.
- [ ] Independent byte-vector tests verify the encoded field.

## Sub-Issue 2.2: Enforce packet boundaries during deserialization

### Description

Decode the primary header first and use Packet Data Length to determine the exact packet boundary.

### Acceptance Criteria

- [ ] Expected packet size is calculated as `6 + Packet Data Length + 1`.
- [ ] Truncated packets return a specific error.
- [ ] Concatenated packets can be parsed one at a time.
- [ ] Trailing bytes are not consumed as part of the packet.
- [ ] Parsing reports the number of bytes consumed.
- [ ] No out-of-bounds reads are possible.

## Sub-Issue 2.3: Make packet error control optional

### Description

Remove the assumption that all packets contain a two-byte CRC and introduce an explicit packet error control mode.

### Acceptance Criteria

- [ ] `None` and `CRC16` modes are supported.
- [ ] Serialization does not append CRC when disabled.
- [ ] Deserialization does not infer CRC from remaining bytes.
- [ ] Packet-size calculation reflects the configured mode.
- [ ] Configuration files expose packet error control explicitly.

## Sub-Issue 2.4: Correct CRC calculation coverage

### Description

Calculate CRC over the complete packet content preceding the CRC field: primary header, secondary header, and application data.

### Acceptance Criteria

- [ ] CRC coverage starts at the first primary-header byte.
- [ ] CRC coverage ends immediately before the CRC field.
- [ ] CRC output matches independent reference vectors.
- [ ] CRC parameters remain configurable where supported.
- [ ] CRC serialization is big-endian.

## Sub-Issue 2.5: Validate CRC during packet parsing

### Description

Extract the received CRC, calculate the expected CRC, and compare the two when packet error control is enabled.

### Acceptance Criteria

- [ ] Valid CRC packets pass.
- [ ] Corrupted packets return a dedicated CRC mismatch error.
- [ ] CRC-free packets skip CRC validation.
- [ ] Validation does not modify packet contents.
- [ ] One-bit corruption tests are included.

## Sub-Issue 2.6: Correct Packet Sequence Count behaviour

### Description

Remove the behaviour that forces unsegmented packets to sequence count zero.

### Acceptance Criteria

- [ ] Unsegmented packets support non-zero sequence counts.
- [ ] Sequence counts increment modulo 16384.
- [ ] Sequence count rollover is tested.
- [ ] Parsed packets preserve the received sequence count.
- [ ] Automatic counter updates are explicitly configurable.

## Sub-Issue 2.7: Maintain independent sequence counters per APID

### Description

Update the higher-level manager to maintain independent sequence counters per APID while keeping the low-level packet codec stateless.

### Acceptance Criteria

- [ ] Different APIDs use independent counters.
- [ ] Counter state is not stored globally in the packet codec.
- [ ] Counter rollover is handled correctly.
- [ ] Manual sequence-count mode remains supported.
- [ ] Counter behaviour is documented.

## Sub-Issue 2.8: Correct APID type and range validation

### Description

Fix API and configuration paths that store APID using an eight-bit value.

### Acceptance Criteria

- [ ] APID is stored using at least `std::uint16_t`.
- [ ] Values from 0 to 2046 are supported as normal APIDs.
- [ ] APID 2047 is handled explicitly as the idle APID.
- [ ] Values above 2047 return an error.
- [ ] Configuration loading does not truncate APIDs.

## Sub-Issue 2.9: Replace silent field masking with explicit validation

### Description

Replace public setter and constructor masking with checked assignment so invalid input cannot silently become a different valid value.

### Acceptance Criteria

- [ ] Invalid APIDs return an error.
- [ ] Invalid sequence counts return an error.
- [ ] Invalid sequence flags return an error.
- [ ] Invalid packet versions return an error.
- [ ] Invalid packet types return an error.
- [ ] Internal decoding assigns fields only after validation.

---

# Main Issue 3: Redesign PUS Secondary Header Support

## Description

Replace the current PUS-A, PUS-B, and PUS-C model with standards-oriented TC and TM secondary-header implementations.

PUS revision and packet direction must be represented independently.

## Acceptance Criteria

- [ ] TC and TM headers are separate concrete types.
- [ ] PUS revision is explicitly represented.
- [ ] The invalid PUS-B model is removed.
- [ ] PUS-C TC and TM headers match the selected standard.
- [ ] PUS-A support is correctly implemented or explicitly deferred.
- [ ] PUS-specific fields are not mixed with custom application fields.

## Sub-Issue 3.1: Introduce explicit PUS revision and packet direction types

### Description

Add strong types for PUS revision and packet direction, replacing string-only identification.

### Acceptance Criteria

- [ ] A `PusRevision` type exists.
- [ ] Packet direction is explicit.
- [ ] TC and TM secondary headers cannot be confused.
- [ ] Factory selection uses structured identifiers rather than free-form strings.
- [ ] Unsupported combinations return an error.

## Sub-Issue 3.2: Separate PUS TC and TM secondary-header abstractions

### Description

Replace the generic PUS secondary-header abstraction with direction-specific types.

### Acceptance Criteria

- [ ] TC and TM headers expose different required fields.
- [ ] A TC header cannot be attached to a TM packet.
- [ ] A TM header cannot be attached to a TC packet.
- [ ] Serialization validates packet-type compatibility.
- [ ] Parsing requires the expected header direction.

## Sub-Issue 3.3: Implement the PUS-C TC secondary header

### Description

Implement the PUS-C telecommand secondary header according to ECSS-E-ST-70-41C and the selected mission profile.

### Acceptance Criteria

- [ ] First-octet bit layout is correct.
- [ ] All acknowledgement flags are independently represented.
- [ ] Service type and subtype are encoded correctly.
- [ ] Source-ID width is profile-driven.
- [ ] Reserved bits are validated.
- [ ] No custom application-data length field is present.
- [ ] Independent reference vectors pass.

## Sub-Issue 3.4: Implement the PUS-C TM secondary header

### Description

Implement the PUS-C telemetry secondary header according to ECSS-E-ST-70-41C and the selected mission profile.

### Acceptance Criteria

- [ ] First-octet bit layout is correct.
- [ ] Time-reference status is represented.
- [ ] Message-type counter is supported.
- [ ] Destination-ID width is profile-driven.
- [ ] Optional timestamp is profile-driven.
- [ ] Reserved bits are validated.
- [ ] No custom application-data length field is present.
- [ ] Independent reference vectors pass.

## Sub-Issue 3.5: Implement CCSDS time-field support for PUS-C TM

### Description

Replace the arbitrary time-code byte vector with an explicitly configured CCSDS time representation.

### Acceptance Criteria

- [ ] Time format is selected through the mission profile.
- [ ] Coarse-time length is configurable.
- [ ] Fine-time length is configurable.
- [ ] Epoch handling is documented.
- [ ] Encoded time size is deterministic.
- [ ] Invalid time configurations return an error.
- [ ] Encode and decode reference vectors are included.

## Sub-Issue 3.6: Implement the PUS-A TC secondary header

### Description

Implement the PUS-A telecommand secondary header according to the selected historical PUS-A reference.

### Acceptance Criteria

- [ ] TC-specific PUS-A fields are implemented.
- [ ] Field layout maps to documented PUS-A clauses.
- [ ] Mission-tailored fields are profile-driven.
- [ ] The current custom data-length field is removed.
- [ ] Independent reference vectors pass.

## Sub-Issue 3.7: Implement the PUS-A TM secondary header

### Description

Implement the PUS-A telemetry secondary header according to the selected historical PUS-A reference.

### Acceptance Criteria

- [ ] TM-specific PUS-A fields are implemented.
- [ ] TC and TM layouts are distinct.
- [ ] Field layout maps to documented PUS-A clauses.
- [ ] Mission-tailored fields are profile-driven.
- [ ] Independent reference vectors pass.

## Sub-Issue 3.8: Remove the invalid PusB secondary header

### Description

Remove `PusB` from standards-facing APIs, documentation, configuration, tests, examples, and factory registration.

### Acceptance Criteria

- [ ] `PusB` class is removed.
- [ ] PUS-B configuration is removed.
- [ ] PUS-B diagrams and examples are removed.
- [ ] PUS-B factory registration is removed.
- [ ] Repository documentation no longer describes PUS-B as a standard.
- [ ] A source search for standards-facing `PusB` references returns no result.

## Sub-Issue 3.9: Decide the migration path for the existing PusB wire format

### Description

Determine whether the current custom PusB format is deleted or retained under an explicitly proprietary name.

### Acceptance Criteria

- [ ] A decision is documented.
- [ ] If retained, the type is clearly marked proprietary.
- [ ] The type is excluded from compliance claims.
- [ ] Migration instructions are provided.
- [ ] Event identifiers are documented as service-specific application data where applicable.

---

# Main Issue 4: Introduce Mission Profiles and Deterministic Packet Finalization

## Description

Introduce a mission-profile model for tailored fields and replace hidden packet mutation with explicit packet finalization.

## Acceptance Criteria

- [ ] Mission-tailored values are stored in one validated profile.
- [ ] Generic CCSDS packets work without a PUS profile.
- [ ] PUS parsing requires an appropriate profile.
- [ ] Getters no longer modify packet state.
- [ ] Packet finalization is explicit and deterministic.

## Sub-Issue 4.1: Add a mission profile model

### Description

Add a profile structure containing packet-format choices required for serialization and parsing.

### Acceptance Criteria

- [ ] Profile structure is publicly documented.
- [ ] A default generic CCSDS profile exists.
- [ ] PUS profiles require explicit revision selection.
- [ ] Invalid combinations return a profile validation error.
- [ ] Header sizes are derived from the profile.

## Sub-Issue 4.2: Add mission profile validation

### Description

Implement validation for incompatible or unsupported profile combinations.

### Acceptance Criteria

- [ ] Invalid profiles cannot be used for serialization.
- [ ] Invalid profiles cannot be used for strict parsing.
- [ ] Validation returns specific error messages.
- [ ] All profile constraints have unit tests.
- [ ] Profile validation does not modify the profile.

## Sub-Issue 4.3: Refactor secondary-header factory selection

### Description

Replace free-form string selection with structured selection based on PUS revision, packet direction, secondary-header type, and mission profile.

### Acceptance Criteria

- [ ] Factory keys are strongly typed.
- [ ] Invalid revision and direction combinations fail.
- [ ] Custom secondary headers remain extensible.
- [ ] PUS headers cannot be selected solely using an ambiguous string.
- [ ] Existing configuration is migrated.

## Sub-Issue 4.4: Introduce explicit packet finalization

### Description

Add an explicit operation that prepares a packet for serialization.

Finalization must validate consistency, update dependent fields, calculate Packet Data Length, and calculate CRC when enabled.

### Acceptance Criteria

- [ ] `finalize()` or equivalent exists.
- [ ] Finalization returns validation errors.
- [ ] Getters do not trigger finalization.
- [ ] Parsed packets are not automatically modified.
- [ ] Serialization behaviour is documented as requiring or invoking finalization.

## Sub-Issue 4.5: Remove hidden mutation from getters

### Description

Refactor getters so they do not silently alter packet state.

### Acceptance Criteria

- [ ] Read-only getters are `const` where appropriate.
- [ ] Calling a getter does not change sequence count.
- [ ] Calling a getter does not recalculate length.
- [ ] Calling a getter does not recalculate CRC.
- [ ] Parsed packet contents remain unchanged during inspection.

---

# Main Issue 5: Strengthen Validation and Conformance Testing

## Description

Add strict protocol validation and independent conformance evidence.

## Acceptance Criteria

- [ ] Validation covers all supported CCSDS and PUS fields.
- [ ] Independent golden vectors exist.
- [ ] Negative vectors cover malformed packets.
- [ ] CRC corruption is detected.
- [ ] Sanitizer and fuzz test jobs pass.
- [ ] Compliance documentation references tests.

## Sub-Issue 5.1: Extend the packet validator

### Description

Update the validator to check all supported v2 protocol requirements.

### Acceptance Criteria

- [ ] Validation returns specific failure reasons.
- [ ] Validation does not modify the packet.
- [ ] Strict validation mode is available.
- [ ] Unsupported packet profiles return a clear error.
- [ ] Each validation path has a test.

## Sub-Issue 5.2: Add independent CCSDS golden vectors

### Description

Add fixed byte vectors for valid CCSDS Space Packets calculated independently from CCSDSPack.

### Acceptance Criteria

- [ ] Encode output matches expected bytes exactly.
- [ ] Decode output matches expected logical fields.
- [ ] Packet Data Length is independently verified.
- [ ] CRC is independently verified.
- [ ] Tests run on Linux and Windows.

## Sub-Issue 5.3: Add independent PUS-C golden vectors

### Description

Add fixed reference vectors for PUS-C telecommand and telemetry packets.

### Acceptance Criteria

- [ ] Header bytes match independent references.
- [ ] Field extraction matches expected values.
- [ ] Reserved bits are verified.
- [ ] Identifier widths are verified.
- [ ] Time-field bytes are verified.

## Sub-Issue 5.4: Add PUS-A golden vectors

### Description

Add fixed reference vectors for PUS-A TC and TM packets if PUS-A remains in v2 scope.

### Acceptance Criteria

- [ ] PUS-A TC vectors pass.
- [ ] PUS-A TM vectors pass.
- [ ] PUS-A and PUS-C packets are distinguishable.
- [ ] PUS-A vectors reference the selected standard and profile.
- [ ] Tests are omitted only if PUS-A is formally deferred.

## Sub-Issue 5.5: Add negative packet validation vectors

### Description

Add malformed packet vectors covering parser and protocol failures.

### Acceptance Criteria

- [ ] Every malformed vector fails for the expected reason.
- [ ] No malformed input causes an over-read.
- [ ] No malformed input triggers excessive allocation.
- [ ] Validation failures are deterministic.
- [ ] Tests cover direct parsing and validator APIs.

## Sub-Issue 5.6: Add sanitizer and fuzz testing

### Description

Add automated robustness testing for packet and secondary-header parsers.

### Acceptance Criteria

- [ ] AddressSanitizer job passes.
- [ ] UndefinedBehaviorSanitizer job passes.
- [ ] A fuzz target exists for primary-header parsing.
- [ ] A fuzz target exists for packet parsing.
- [ ] A fuzz target exists for PUS secondary-header parsing.
- [ ] CI runs a bounded fuzz smoke test.

---

# Main Issue 6: Migrate Public APIs, Tools, Documentation, and Release Packaging

## Description

Migrate the user-facing library components to the compliant v2 protocol model.

## Acceptance Criteria

- [ ] Public APIs no longer expose invalid PUS concepts.
- [ ] CLI tools use compliant packet encoding and decoding.
- [ ] Existing incorrect configurations fail clearly.
- [ ] Documentation contains accurate compliance claims.
- [ ] v2 release notes identify wire and API incompatibilities.
- [ ] Packages and CI validate installed consumers.

## Sub-Issue 6.1: Migrate packet configuration files

### Description

Replace the existing secondary-header configuration model with explicit PUS revision, packet direction, mission profile, packet error control, identifier widths, and time format.

### Acceptance Criteria

- [ ] New v2 configuration format is documented.
- [ ] PUS-B configuration is removed.
- [ ] Invalid old configuration returns a migration error.
- [ ] Example configurations exist for generic CCSDS, PUS-C TC, and PUS-C TM.
- [ ] PUS-A examples exist if PUS-A is supported.

## Sub-Issue 6.2: Migrate CCSDS Manager to compliant sequence and packet handling

### Description

Update `CCSDS::Manager` to use the corrected packet model.

### Acceptance Criteria

- [ ] Manager maintains sequence counters per APID.
- [ ] Manager does not force unsegmented sequence counts to zero.
- [ ] Manager uses explicit packet finalization.
- [ ] Segmentation behaviour is documented.
- [ ] Packet generation uses the mission profile.

## Sub-Issue 6.3: Migrate the encoder CLI

### Description

Update the encoder executable to generate compliant generic CCSDS and PUS packets.

### Acceptance Criteria

- [ ] Encoder accepts the v2 profile configuration.
- [ ] Packet Data Length is correct.
- [ ] CRC mode is configurable.
- [ ] PUS revision and direction are explicit.
- [ ] Output matches golden-vector expectations.

## Sub-Issue 6.4: Migrate the decoder CLI

### Description

Update the decoder executable to parse packet boundaries using encoded Packet Data Length.

### Acceptance Criteria

- [ ] Multiple concatenated packets are decoded individually.
- [ ] Truncated input is reported clearly.
- [ ] CRC validation results are displayed.
- [ ] PUS revision and direction are profile-driven.
- [ ] Decoder does not silently consume trailing bytes.

## Sub-Issue 6.5: Migrate the validator CLI

### Description

Update the validator executable to expose the complete v2 validation result.

### Acceptance Criteria

- [ ] Length failures are reported.
- [ ] CRC failures are reported.
- [ ] Header/profile mismatches are reported.
- [ ] PUS field failures are reported.
- [ ] Exit codes distinguish valid packets, validation failures, and tool errors.

## Sub-Issue 6.6: Update API examples and diagrams

### Description

Replace examples and diagrams describing the current custom PUS-A/B/C model.

### Acceptance Criteria

- [ ] Generic CCSDS packet example is updated.
- [ ] PUS-C TC example is added.
- [ ] PUS-C TM example is added.
- [ ] PUS-B diagrams are removed.
- [ ] Packet field diagrams match actual serialized layouts.
- [ ] Examples compile in CI.

## Sub-Issue 6.7: Add the v1-to-v2 migration guide

### Description

Document API, configuration, and wire-format changes required when moving from v1 to v2.

### Acceptance Criteria

- [ ] `docs/MIGRATION_V1_TO_V2.md` is added.
- [ ] Each breaking change includes a before-and-after example.
- [ ] Legacy packet incompatibility is clearly stated.
- [ ] Removed APIs list their replacement.
- [ ] Configuration migration examples are included.

## Sub-Issue 6.8: Update README and compliance claims

### Description

Rewrite the README to accurately state supported standards and limitations.

### Acceptance Criteria

- [ ] Exact supported standards are named.
- [ ] CCSDS Space Packet support is distinguished from transfer frames.
- [ ] PUS-C support scope is described.
- [ ] PUS-A support scope is described or marked deferred.
- [ ] PUS-B is not described as a standard.
- [ ] Unsupported protocol layers are listed.
- [ ] Compliance claims link to the compliance matrix.

## Sub-Issue 6.9: Update CI and package validation for v2

### Description

Update build and test pipelines for v2 API and protocol changes.

### Acceptance Criteria

- [ ] Linux builds pass.
- [ ] Windows builds pass.
- [ ] MCU cross-build passes.
- [ ] Installed package consumer tests pass.
- [ ] Sanitizer jobs pass.
- [ ] Golden-vector tests run in CI.
- [ ] CLI integration tests run in CI.

## Sub-Issue 6.10: Prepare the v2.0.0 release

### Description

Prepare version metadata, release notes, packaging, and final compliance evidence.

### Acceptance Criteria

- [ ] Project version is `2.0.0`.
- [ ] Shared-library versioning reflects the breaking release.
- [ ] Release notes enumerate all breaking changes.
- [ ] Compliance matrix is complete.
- [ ] All milestone issues are closed or explicitly deferred.
- [ ] Release packages contain updated headers, documentation, and tools.
- [ ] Tag `v2.0.0` is created only after all release gates pass.

---

# Release Gate

CCSDSPack v2.0.0 may be released only when:

- [ ] All mandatory issues in this document are complete.
- [ ] Deferred issues are explicitly documented and removed from release claims.
- [ ] Generic CCSDS Space Packet golden vectors pass.
- [ ] PUS-C TC and TM golden vectors pass.
- [ ] CRC and packet-length vectors pass.
- [ ] Negative parser and validator tests pass.
- [ ] Linux, Windows, sanitizer, and MCU jobs pass.
- [ ] README and compliance documentation reflect actual implementation status.
- [ ] The release does not expose `PusB` as an official protocol concept.
