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
- [x] All 26 public `ValidationCode` entries are mapped to release evidence.

## Current integration evidence

- [x] **132/132 native tests** pass on the evidence-hardening candidate.
- [x] Linux Ubuntu 22.04/24.04/latest hosted gates pass.
- [x] Windows latest hosted gate passes.
- [x] Doxygen passes.
- [x] CLI integration passes.
- [x] Installed shared-library consumer passes.
- [x] Installed-package examples pass on Linux and Windows.
- [x] Ubuntu 22.04 native/package/cross-build generation passes.
- [x] Cortex-M compile/link probe covers Packet, PUS, raw-buffer, and Validator APIs.
- [x] Dedicated Clang ASan CI passes the complete native suite.
- [x] Dedicated Clang UBSan CI passes the complete native suite.
- [x] Bounded four-target libFuzzer smoke CI passes under ASan+UBSan.
- [x] Complete PUS-C TC acknowledgement matrix `0x0..0xF` is independently fixed and traced to ECSS-E-ST-70-41C clause 7.4.4.1.
- [x] Final structured negative-validation evidence matrix is documented.
- [x] Active hosted workflows target only `main` and `develop`; the retired v2 staging branch is no longer referenced.
- [x] Tag publication uses the current root `RELEASE_NOTES.md` and no v1.2 release-note file is carried in the v2 tree.
- [x] Hardware-validation CI artifact naming is version-neutral.
- [x] v1.2-to-v2 migration guide is consolidated against the final API.
- [x] Current-facing documentation describes the final v2 model without migration history.

## Release gates

- [x] Fresh native arm64 v2 package/API execution is recorded.
- [x] Fresh physical NUCLEO-H755ZI-Q / Cortex-M7 PUS/Validator/raw-buffer execution is recorded.
- [x] Tag-only GitHub Release / GHCR publication behavior is verified on the final release path.
- [x] Final compliance evidence includes publication results in addition to the recorded arm64/STM32 results.
- [x] Final release notes are approved after all pre-tag gates pass.
- [x] Approved `develop` is promoted to `main` after release-hardening gates are complete.
- [x] Final `main` CI passes.
- [x] `v2.0.0` is tagged from the approved `main` commit.
- [x] Published release packages/artifacts are verified.

## Publication record

The annotated `v2.0.0` tag resolves to approved `main` commit `c2f318c330c564429bcc565a8acbff22728b2851`.

The canonical tag publication workflow completed successfully and published:

- `ccsdspack-v2.0.0-Linux-x86_64.deb`, SHA-256 `779841b9f5705af56bcac2a6cb014261ff30711642a0334e0a3ad9bb1f86e22f`;
- `ccsdspack-v2.0.0-Linux-arm64.deb`, SHA-256 `8e31a2a2d8f80c2604f4457e9388d66a49f79a228c8d65603542a4e5e9d5bdb3`;
- `ccsdspack-v2.0.0-Generic-arm.tar.gz`, SHA-256 `1d4d3fdab567b8f52f34a532af37917913ba2be7c1bacf3dec0dd098cdb457ae`;
- GHCR images `ghcr.io/exospacelabs/ccsdspack:v2.0.0` and `ghcr.io/exospacelabs/ccsdspack:latest`.

GitHub Release creation, release-asset upload, GHCR login, exact-tag image build, and both image pushes completed successfully in the release workflow.
