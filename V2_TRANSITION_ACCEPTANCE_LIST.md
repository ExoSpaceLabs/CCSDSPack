# CCSDSPack v2.1.0 release acceptance list

This checklist tracks substantive pre-release evidence for v2.1.0. The C-core
migration is accepted only if the v2 standards claim remains unchanged and the
existing independent conformance evidence continues to pass.

## Standards and compatibility

- [x] CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 Space Packet PDU profile preserved.
- [x] ECSS-E-70-41A PUS-A TC/TM secondary-header subset preserved.
- [x] ECSS-E-ST-70-41C PUS-C TC/TM secondary-header subset preserved.
- [x] CCSDS 301.0-B-4 basic numeric CUC subset preserved.
- [x] Existing C++17 Packet/Manager API remains available.
- [x] C11 core is available independently through `ccsdspack::c`.
- [x] PUS revision/direction identity and direction-specific tailoring remain enforced.
- [x] Packet-level PEC remains independent of secondary-header type.
- [x] Independent fixed wire vectors remain byte-identical.
- [x] Complete PUS-C TC acknowledgement matrix `0x0..0xF` remains independently verified.
- [x] All 26 public `ValidationCode` entries remain traced to release evidence.

## Integration and robustness

- [x] **134/134 native regression/conformance tests** pass on the v2.1 candidate.
- [x] Linux Ubuntu 22.04/24.04/latest hosted gates pass.
- [x] Windows hosted gate passes.
- [x] Doxygen passes.
- [x] CLI integration passes.
- [x] Installed C and C++ consumers/examples pass.
- [x] C-only configuration works without a C++ compiler/runtime.
- [x] Native/cross-package generation passes.
- [x] Cortex-M compile/link probe passes.
- [x] Clang ASan and UBSan gates pass.
- [x] Bounded libFuzzer smoke gates pass.

## Performance evidence

- [x] Matched v2.0-main vs v2.1 owning raw-parse benchmark recorded.
- [x] Matched v2.0-main vs v2.1 packet-serialization benchmark recorded.
- [x] Allocation counts and allocated bytes/op recorded.
- [x] Cortex-M7 retained-text comparison recorded.
- [ ] Complete broader matched PUS-A/PUS-C, CUC, Validator, Manager,
  segmentation/reassembly, stream-walking, and isolated-CRC characterization in issue #163.
- [ ] Update `docs/PERFORMANCE.md` with the issue #163 final artifacts/results before
  claiming whole-core performance improvement.

## Hardware validation

- [x] Raspberry Pi 5 native arm64 package/API execution recorded.
- [x] Raspberry Pi runner records platform/source/package identity and hashes.
- [x] NUCLEO-H755ZI-Q Cortex-M7 standalone DAS/OpenOCD validation recorded.
- [x] H755 hard-float startup path physically qualified.
- [x] Hardware validation evidence is recorded in `docs/V2_HARDWARE_VALIDATION.md`.

## Final release gates

- [ ] Issue #163 performance characterization complete.
- [ ] Final release/compliance documentation merged into `develop`.
- [ ] Approved `develop` promoted to `main`.
- [ ] Final `main` Linux, Windows, Doxygen, and robustness workflows pass.
- [ ] `v2.1.0` tag created from the approved `main` commit.
- [ ] Tag-produced GitHub Release assets and package hashes verified.
- [ ] GHCR `v2.1.0` and `latest` images verified.

## Historical v2.0 publication

The v2.0.0 publication record is intentionally not duplicated here. It remains
available in the v2.0.0 GitHub Release, repository history, and the historical
hardware-validation records.
