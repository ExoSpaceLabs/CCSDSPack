<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v2.0.0 hardware validation

[Documentation index](README.md) | [Packages](PACKAGES.md) | [Release acceptance](../V2_TRANSITION_ACCEPTANCE_LIST.md)

This page records physical-target and native-target release evidence for CCSDSPack v2.0.0. Hardware execution complements hosted CI and package/cross-build evidence; it does not extend the documented compliance scope beyond the supported CCSDS Space Packet PDU, PUS, CUC, and mission-tailoring profiles.

## Status

| Target | Status | Required marker |
|---|---|---|
| Raspberry Pi 5, native arm64 Linux | **PASS** | `CCSDSPACK_AARCH64_TEST:PASS` |
| STM32H755ZITx / NUCLEO-H755ZI-Q CM7 | Pending | `CCSDSPACK_HARDWARE_TEST:PASS` |

## Raspberry Pi 5 v2 validation record

Validation date: **2026-08-31**

### Platform

- Board: Raspberry Pi 5
- Architecture: `aarch64`
- Operating system: Debian GNU/Linux 13.5 (`trixie`), 64-bit
- Kernel: `6.18.34+rpt-rpi-2712`
- C++ compiler: GNU C++ 14.2.0
- CMake: 3.31.6

### Candidate and package

- Branch: `develop`
- Source commit: `50a4fadaf5347223c16330efe8e65f5261c96959`
- Source state: clean working tree
- Package: `ccsdspack`
- Package version: `2.0.0`
- Package architecture: `arm64`
- Package file: `ccsdspack-v2.0.0-Linux-arm64.deb`
- SHA-256: `38fdf43eb7d9d28a9cfea57abc855ec814c66597653f0dd0fa20d63cd833a75a`

Before the v2 run, the previous v1.2 package was no longer installed: the package database contained only a removed/config-files residual entry, with no installed package files, CCSDSPack executables in `PATH`, or registered CCSDSPack shared libraries. The v2 candidate was then cloned fresh from `develop` and packaged natively on the target.

### Validation procedure

The native package was built with:

```bash
./package.sh -p DEB
```

Package metadata was checked with `dpkg-deb` before installation and reported `ccsdspack`, version `2.0.0`, architecture `arm64`.

The complete target validation was executed as an ordinary user with:

```bash
bash test/package_tester/aarch64_validate.sh "$ARM64_DEB"
```

The validation harness installed the package through `dpkg`, ran the installed regression suite and CLI integration suite, configured and built an external installed-package CMake consumer, ran its CTest, and executed the board-independent hardware acceptance body shared with the STM32 validation path.

### Results

The complete native arm64 validation passed:

- installed regression/conformance suite: **132 passed, 0 failed**;
- installed encoder/decoder/validator CLI integration: **PASS**;
- external installed-package CMake consumer configured and linked successfully;
- external consumer CTest: **1 passed, 0 failed**;
- shared board-independent acceptance body: `CCSDSPACK_HARDWARE_TEST:PASS`;
- final native arm64 marker: `CCSDSPACK_AARCH64_TEST:PASS`.

The installed package was then re-queried and reported:

```text
ccsdspack 2.0.0 arm64 install ok installed
```

Installed release surfaces confirmed by the package include:

- `ccsds_encoder`, `ccsds_decoder`, and `ccsds_validator`;
- `libccsdspack.so`, SOVERSION `2`, and `libccsdspack.so.2.0.0`;
- `CCSDSPackConfig.cmake`, `CCSDSPackConfigVersion.cmake`, and exported CMake targets.

### Acceptance

This run satisfies the v2.0.0 release gate requiring fresh native arm64 installed-package/API execution against the final `develop` hardware-acceptance baseline. It establishes execution on the tested Raspberry Pi 5 / Debian 13.5 arm64 environment for the exact source commit and package hash recorded above.

The remaining physical-target hardware gate is execution of the same shared acceptance body on NUCLEO-H755ZI-Q / STM32H755 CM7, together with the required target identity and memory/fault evidence.
