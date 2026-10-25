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

- [x] **137/137 hosted native regression/conformance tests** passed on the pre-final `develop` CI baseline (Ubuntu 22.04 Linux workflow, 2026-10-23). A subsequent diagnostic formatter fix adds a regression case; final-source CI remains unchecked below. The October 11 Pi 5 candidate passed 134/134 tests, now historical.
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

## Historical hardware validation (2026-10-11 candidate)

- [x] Raspberry Pi 5 native arm64 package/API execution recorded.
- [x] Raspberry Pi runner records platform/source/package identity and hashes.
- [x] NUCLEO-H755ZI-Q Cortex-M7 standalone DAS/OpenOCD validation recorded.
- [x] H755 hard-float startup path physically qualified.
- [x] Pi 5 and H755 historical hardware runs passed on the same previous `develop` implementation commit `3c93e6b79d3d97ff3003682804c120f38672e229` (2026-10-11).
- [x] Historical package hashes, MCU library/ELF hashes, full target PASS markers, firmware size and heap statistics recorded in `docs/V2_HARDWARE_VALIDATION.md`.


## Post-audit requalification

- [x] MCU-facing public APIs, conditional installed-header package rules, and capacity-underflow regression fixes merged into `develop` in PR #177.
- [x] Linux, Windows, Doxygen and robustness CI passed on post-merge `develop` implementation `f076991b88cf70e45a1752b8ea8dd81878ba6a38`.
- [x] Final `develop` Linux, Windows, Doxygen and robustness CI passed on exact implementation `8d6fe89a946fafd2be0d48917aa77fe473f89ecf`.
- [x] Fresh Raspberry Pi 5 native arm64 package validation passed on `8d6fe89a946fafd2be0d48917aa77fe473f89ecf` (2026-10-25): 138/138 native tests, CLI, installed consumer and all acceptance markers PASS; DEB SHA-256 `8974910ddf7ac90acbfd9e6b7353d65fa94058eaeff19b169b0130ab71871515`. Evidence in `docs/V2_HARDWARE_VALIDATION.md`.
- [x] Fresh STM32H755 physical Cortex-M7 validation **PASS** on the same implementation `8d6fe89a946fafd2be0d48917aa77fe473f89ecf` (2026-10-25), all 8 acceptance sections and OpenOCD flash verification PASS. MCU package SHA-256 `4a9ab0156d04df62305031dc5e4ea32438b24c5aae0d610ac4f4b1f023ad7a9c`, library SHA-256 `50369efaa72ab093469184a1be655a8d04d4cb6a76dfe8e81a69e4606294120c`, ELF SHA-256 `96726120004288ac61f9daa076eb63b955be8a06fdf0d01dd221cf52e2779800`. Full metadata/UART/OpenOCD/size evidence reviewed; see `docs/V2_HARDWARE_VALIDATION.md`.
- [ ] Only then promote to main and allow automatic VERSION tagging.

## Final release gates

**Historical qualification vs current candidate:** all October 11 target PASS markers below apply to `3c93e6b` only. The current release has independently **passed both** post-audit Pi 5/H755 gates above on `8d6fe89`; the historical checkboxes were not used to authorize release. Tagging is still gated on reviewed `develop -> main` promotion and final CI.

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
