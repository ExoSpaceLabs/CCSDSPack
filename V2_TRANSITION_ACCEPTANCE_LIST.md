# CCSDSPack v2.0.0 release acceptance list

This checklist tracks substantive release evidence for the implemented v2.0.0 design. Branch promotion status is recorded by GitHub pull requests rather than duplicated in this static document.

## Standards and API

- [x] CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 Space Packet PDU profile documented.
- [x] ECSS-E-70-41A PUS-A TC/TM secondary-header subset implemented.
- [x] ECSS-E-ST-70-41C PUS-C TC/TM secondary-header subset implemented.
- [x] CCSDS 301.0-B-4 basic numeric CUC subset implemented.
- [x] Public protocol API is C++17 under `ccsds`.
- [x] Packet-level PEC and CRC configuration are independent of secondary-header type.
- [x] Concrete PUS types own revision and TC/TM direction.
- [x] Optional PUS tailoring is direction-specific and validated.
- [x] Manager uses one complete Packet template and one sequence stream.
- [x] Checked Result-based finalization/serialization is used by Packet, DataField, and Manager.

## Parsing and validation

- [x] Bounded transactional parsing reports consumed bytes.
- [x] Preinstalled, typed, runtime-selector, and raw-buffer PUS parsing are supported.
- [x] `declaredPacketSize()` supports six-byte transport framing decisions.
- [x] Structured `ValidationReport` uses named checks and fixed storage.
- [x] Sequence validation handles segmentation and modulo-16384 continuity.
- [x] Validator is available in `CCSDS_MCU` with no RTTI/exception requirement.

## Current integration evidence

- [x] 125 native tests pass on the integration candidate.
- [x] Linux Ubuntu 22.04/24.04/latest hosted gates pass.
- [x] Windows latest hosted gate passes.
- [x] Doxygen passes.
- [x] CLI integration passes.
- [x] Installed shared-library consumer passes.
- [x] Installed-package examples pass on Linux and Windows.
- [x] Ubuntu 22.04 native/package/cross-build generation passes.
- [x] Cortex-M compile/link probe covers Packet, PUS, raw-buffer, and Validator APIs.
- [x] Local ASan and UBSan runs have passed.
- [x] v1.2-to-v2 migration guide is consolidated against the final API.
- [x] Current-facing documentation describes the final v2 model without migration history.

## Remaining release gates

- [ ] Dedicated sanitizer/fuzz CI jobs pass.
- [ ] Final PUS-C acknowledgement-vector matrix and traceability are approved.
- [ ] Final structured negative-vector coverage is approved.
- [ ] Fresh native arm64 v2 package/API execution is recorded.
- [ ] Fresh physical STM32H755 v2 PUS/Validator/raw-buffer execution is recorded.
- [ ] Final compliance evidence includes the completed vector/fuzz/hardware results.
- [ ] Final release notes are approved after all gates pass.
- [ ] Approved `develop` is promoted to `main` after release-hardening gates are complete.
- [ ] Final `main` CI passes.
- [ ] `v2.0.0` is tagged from the approved `main` commit.
- [ ] Published release packages/artifacts are verified.
