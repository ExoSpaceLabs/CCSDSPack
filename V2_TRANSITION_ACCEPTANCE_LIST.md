# CCSDSPack v2.0.0 transition acceptance list

## Baseline

- [x] v1.2.0 generic Space Packet implementation synchronized into `v2.0.0-dev`.
- [x] CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 selected.
- [x] ECSS-E-70-41A selected for PUS-A secondary headers.
- [x] ECSS-E-ST-70-41C selected for PUS-C secondary headers.
- [x] PUS-B rejected as a standards revision.

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
- [x] TM version and time-reference status are correct.
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

## Legacy removal and migration

- [x] Legacy project-specific `PusA`, `PusB`, and `PusC` classes removed.
- [x] `PusServices.h/.cpp` removed.
- [x] Automatic legacy registration removed.
- [x] Legacy configuration selectors rejected with migration guidance.
- [x] Current tests and diagrams no longer present legacy codecs as supported.
- [x] `docs/MIGRATION_V1_TO_V2.md` documents API, selector, and wire changes.
- [x] Public secondary-header APIs and configuration use CCSDS secondary-header terminology.
- [x] Project version and SOVERSION set to 2.0.0/2.

## Local validation

- [x] 106 native tests pass.
- [x] AddressSanitizer and UndefinedBehaviorSanitizer pass.
- [x] MCU sources compile with `-fno-exceptions -fno-rtti`.
- [x] `git diff --check` passes.

## Integration and release gates

- [x] `v2.0.0-dev` merged into `develop` through PR #111.
- [x] Linux, Windows, and Doxygen CI pass on `develop`.
- [x] Installed-consumer/package gates use v2 version expectations.
- [x] Standalone generic, custom-header, PUS-C TC, and PUS-C TM examples consume the installed package in Linux and Windows CI.
- [x] Encoder, decoder, and validator accept/report complete PUS profiles.
- [x] Manager higher-level PUS profile workflows are covered.
- [ ] Fuzz smoke jobs pass.
- [ ] arm64 and STM32 representative PUS validation is recorded.
- [ ] Release notes and final compliance traceability are approved.
- [ ] Approved `develop` merged into `main`.
- [ ] `v2.0.0` tag created from approved `main` only.
