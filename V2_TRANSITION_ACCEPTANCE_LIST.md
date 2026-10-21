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

- [x] **134/134 native regression/conformance tests** passed on the 2026-10-11 historical candidate; the current regression suite includes additional post-audit coverage and has passed hosted CI.
- [x] Linux Ubuntu 22.04/24.04 hosted gates pass.
- [x] Linux Ubuntu 26.04 hosted gate passes before v2.1.0 promotion.
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
- [x] Broader matched PUS-A/PUS-C, CUC, Validator, Manager,
  segmentation/reassembly, stream-load, and isolated-CRC characterization completed in issue #163.
- [x] `docs/PERFORMANCE.md` records the final matched artifacts/results, including measured
  regressions rather than claiming a universal whole-core speedup.
- [x] Focused CUC wrapper regression is separately tracked in issue #166 without changing
  the CCSDS/ECSS release claim or correctness gates.

## Hardware validation

- [x] Raspberry Pi 5 native arm64 package/API execution recorded.
- [x] Raspberry Pi runner records platform/source/package identity and hashes.
- [x] NUCLEO-H755ZI-Q Cortex-M7 standalone DAS/OpenOCD validation recorded.
- [x] H755 hard-float startup path physically qualified.
- [x] Final Pi 5 and H755 hardware runs passed on the same post-audit `develop` implementation commit `3c93e6b79d3d97ff3003682804c120f38672e229` (2026-10-11).
- [x] Package hashes, MCU library/ELF hashes, full target PASS markers, firmware size and heap statistics recorded in `docs/V2_HARDWARE_VALIDATION.md`.


## Post-audit requalification

- [x] MCU-facing public APIs, conditional installed-header package rules, and capacity-underflow regression fixes merged into `develop` in PR #177.
- [x] Linux, Windows, Doxygen and robustness CI passed on post-merge `develop` implementation `f076991b88cf70e45a1752b8ea8dd81878ba6a38`.
- [ ] Final `develop` CI rechecked after any further candidate changes and before hardware qualification.
- [ ] Fresh Raspberry Pi 5 native package validation passed on that commit and hashes recorded.
- [ ] Fresh STM32H755 physical validation passed on that commit and hashes recorded.
- [ ] Only then promote to main and allow automatic VERSION tagging.

## Final release gates

**Historical qualification vs current candidate:** all October 11 target PASS markers below apply to `3c93e6b` only. The current release requires the unchecked post-audit Pi 5/H755 gates above; the historical checkboxes do not authorize tagging.

- [x] Issue #163 performance characterization complete.
- [x] Release/compliance documentation audited; PR #169 merged (prematurely) before physical qualification.
- [x] Post-audit qualified implementation SHA fixed: `3c93e6b79d3d97ff3003682804c120f38672e229`.
- [x] `develop` Linux, Windows, and robustness CI passed on the qualified implementation SHA; Doxygen passed on the matching `main` SHA.
- [x] Native Raspberry Pi 5 validation passed on the qualified SHA; ARM64 package SHA-256 recorded.
- [x] Physical NUCLEO-H755ZI-Q validation passed on the qualified SHA; MCU package, library, ELF, UART and OpenOCD evidence recorded.
- [ ] Final documentation-only evidence changes verified against the qualified implementation SHA and accepted on `develop`.
- [ ] Final approved `develop` evidence promoted to `main` without source/build modifications.
- [ ] Final `main` Linux, Windows, Doxygen and robustness workflows pass on the exact commit to be tagged.
- [ ] `v2.1.0` tag created from the approved `main` commit.
- [ ] Tag-produced GitHub Release assets and package hashes verified.
- [ ] GHCR `v2.1.0` and `latest` images verified.
## Historical v2.0 publication

The v2.0.0 publication record is intentionally not duplicated here. It remains
available in the v2.0.0 GitHub Release, repository history, and the historical
hardware-validation records.
