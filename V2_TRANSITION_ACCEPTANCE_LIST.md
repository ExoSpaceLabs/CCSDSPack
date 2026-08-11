# CCSDSPack v2.0.0 transition acceptance list

## Baseline

- [x] v1.2.0 generic Space Packet implementation synchronized into `v2.0.0-dev`.
- [x] CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 selected.
- [x] ECSS-E-70-41A selected for PUS-A secondary headers.
- [x] ECSS-E-ST-70-41C selected for PUS-C secondary headers.
- [x] PUS-B rejected as a standards revision.
- [x] Public implementation remains C++17.

## Mission profile

- [x] Generic CCSDS is valid without enabling PUS.
- [x] PUS requires explicit revision and direction.
- [x] Source and destination identifier widths are validated.
- [x] Packet error control uses one common enum.
- [x] Timestamp presence, format, and encoded size are deterministic.
- [x] PUS-A TM packet-subcounter and spare-byte choices are explicit.
- [x] Invalid combinations return errors without normalization.
- [x] Header sizes derive from the validated profile.
- [x] Basic CUC uses numeric coarse/fine counters with an explicit epoch and P-field policy.
- [x] CUC coarse/fine widths and counter overflow are rejected.

## Factory and type architecture

- [x] PUS revision and TC/TM direction are independent.
- [x] PUS-A TC, PUS-A TM, PUS-C TC, and PUS-C TM are distinct concrete types.
- [x] Custom headers remain direction-neutral and extensible.
- [x] Custom and PUS factories are separate.
- [x] Standards selectors are fixed and cannot be overridden.
- [x] All four canonical selectors are covered through the string factory path.
- [x] Factory creation returns fresh mutable objects.
- [x] Duplicate custom keys and custom `PUS:` keys are rejected.
- [x] Public types use `ccsds`; PUS codecs are grouped under `ccsds::pus::rev_a` and `ccsds::pus::rev_c`.

## PUS-A

- [x] TC reserved/version/acknowledgement fields are correct.
- [x] TC service, subtype, optional source ID, and spare fields are encoded and parsed.
- [x] TM reserved/version/service/subtype fields are correct.
- [x] TM optional subcounter, destination, timestamp, and spare fields are encoded and parsed.
- [x] Fixed positive and malformed negative vectors pass.

## PUS-C

- [x] TC version and acknowledgement fields are correct.
- [x] TC service, subtype, two-octet source ID, and spare fields are encoded and parsed.
- [x] TM version and four-bit time-reference status are correct.
- [x] TM service, subtype, message counter, destination, timestamp, and spare fields are encoded and parsed.
- [x] Fixed vectors with and without timestamp pass.

## Packet integration

- [x] PUS header attachment is a checked operation.
- [x] TC/TM mismatch with primary-header Packet Type is rejected.
- [x] Profile and packet-error-control mismatches are rejected.
- [x] PUS parsing requires the explicit canonical selector.
- [x] Failed PUS parsing does not commit partial packet state.
- [x] Generic CCSDS regression vectors remain passing.
- [x] Packet finalization returns specific validation errors.
- [x] Packet serialization returns `ResultBuffer` rather than using an empty vector as an error sentinel.
- [x] DataField and Manager serialization propagate errors without collapsing them.
- [x] Packet Data Length and CRC16 are updated only after successful checked finalization.

## Structured validation

- [x] `ccsds::Validator::validate()` returns a structured `ValidationReport`.
- [x] Validation failures are named by `ValidationCode` instead of positional boolean indices.
- [x] Generic length, CRC, sequence, Packet Identification, segmentation, and mission-profile checks are represented.
- [x] PUS revision, direction, Packet Type, profile, identifier, reserved/spare, optional-field, and CUC checks are represented.
- [x] Validation does not mutate Packet, MissionProfile, or secondary-header state.
- [x] Sequence validation remains stateful and rolls over modulo 16384.
- [x] `ValidationReport` uses fixed `std::array` storage and performs no dynamic allocation itself.
- [x] The Validator requires neither RTTI nor exceptions and remains part of `CCSDS_MCU`.
- [x] `ccsds_validator` delegates protocol/profile checks to the library Validator.
- [x] Native structured-validator tests cover generic and PUS positive/negative cases.

## Legacy removal and migration

- [x] Legacy project-specific `PusA`, `PusB`, and `PusC` classes removed.
- [x] `PusServices.h/.cpp` removed.
- [x] Automatic legacy registration removed.
- [x] Legacy configuration selectors rejected with migration guidance.
- [x] Current tests no longer present legacy codecs as supported.
- [x] UML diagrams are not treated as current v2 release evidence while generation is deferred.
- [x] `docs/MIGRATION_V1_TO_V2.md` documents API, selector, wire, and structured Validator changes.
- [x] Public secondary-header APIs and configuration use CCSDS secondary-header terminology.
- [x] Project version and SOVERSION set to 2.0.0/2.

## Current validation

- [x] 108 native tests pass in the current Linux build.
- [x] AddressSanitizer and UndefinedBehaviorSanitizer have passed locally.
- [x] MCU sources are designed for `-fno-exceptions -fno-rtti` and the ARM compile/link probe exercises the structured Validator and representative PUS-C TC path.
- [x] Linux and Windows hosted builds compile the structured Validator and CLI.
- [x] Doxygen builds the updated public API documentation.
- [x] Ubuntu 22.04 package/cross-build generation passes with the updated MCU structured-Validator/PUS probe.
- [x] Automatic UML generation is disabled; the workflow remains manual-only through `workflow_dispatch`.

## Integration and release gates

- [x] Linux, Windows, and Doxygen CI are retained as active hosted gates.
- [x] Installed-consumer/package gates use v2 version expectations.
- [x] Standalone generic, custom-header, PUS-C TC, and PUS-C TM examples consume the installed package in Linux and Windows CI.
- [x] Encoder, decoder, and validator accept/report complete PUS profiles.
- [x] Manager higher-level PUS profile workflows are covered.
- [x] Linux package/cross-build gate passes with the updated MCU structured-Validator probe.
- [ ] Dedicated sanitizer/fuzz smoke jobs pass.
- [ ] Final PUS-C acknowledgement-vector matrix and traceability are approved.
- [ ] Final structured negative-vector coverage is approved.
- [ ] arm64 and STM32 representative v2 PUS/Validator execution is recorded.
- [ ] Complete v1.2-to-v2 migration guide is approved.
- [ ] Release notes and final compliance traceability are approved.
- [ ] Approved `v2.0.0-dev` is merged into `develop`.
- [ ] Approved `develop` is merged into `main`.
- [ ] `v2.0.0` tag is created from approved `main` only.
