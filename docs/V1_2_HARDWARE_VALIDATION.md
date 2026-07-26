<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v1.2 hardware validation

[Documentation index](README.md) | [Packages](PACKAGES.md) | [v1.2 release notes](releases/v1.2.0.md)

This page records the release-candidate hardware evidence for CCSDSPack v1.2.0 and provides the repeatable validation procedure. Hardware execution complements the Linux and Windows CI evidence; it does not extend the compliance claim beyond the CCSDS Space Packet PDU profile described in [COMPLIANCE.md](../COMPLIANCE.md).

## Status

| Target | Status | Required marker |
|---|---|---|
| Raspberry Pi 5, native arm64 Linux | **PASS** | `CCSDSPACK_AARCH64_TEST:PASS` |
| STM32H755ZITx / NUCLEO-H755ZI-Q CM7 | Pending | `CCSDSPACK_MCU_TEST:PASS` |

The Raspberry Pi release gate is accepted. The STM32H755 CM7 execution test is the remaining hardware-validation gate.

## Raspberry Pi 5 validation record

Validation date: **2026-07-26**

### Platform

- Board: Raspberry Pi 5
- Host name: `exn-cam`
- Architecture: `aarch64`
- Operating system: Debian GNU/Linux 13.5 (`trixie`), 64-bit
- Kernel: `6.18.34+rpt-rpi-2712`
- C++ compiler used by the external consumer: GNU C++ 14.2.0

### Candidate and package

- Source commit: `761e64d9ac912ea2504b0f1462118ce71c6b56cb`
- Package: `ccsdspack`
- Package version: `1.2.0`
- Package architecture: `arm64`
- Package file: `ccsdspack-v1.2.0-Linux-arm64.deb`
- SHA-256: `066993bd8355fef2c8fa57e0a0058efa7886da52ef84ee7015b93d9480505fdf`

An arm64 DEB transferred to the Raspberry Pi installed successfully and its installed `CCSDSPack_tester` passed all 93 tests. Because the downloadable workflow artifact was not available during the final evidence run, the repository was then checked out at the exact candidate commit and the DEB was rebuilt natively on the Raspberry Pi using the same `package.sh`/CPack packaging path. The complete validation script was run against that package.

### Results

The complete native validation passed:

- installed regression tester: **93 passed, 0 failed**;
- installed encoder/decoder/validator integration suite: **PASS**;
- external CMake consumer resolved the installed package as CCSDSPack `1.2.0` and built successfully;
- external consumer CTest result: **1 passed, 0 failed**;
- final marker: `CCSDSPACK_AARCH64_TEST:PASS`.

The test covers native package installation, dynamic-library loading, packet regression and conformance behaviour, installed command-line tools, CMake package metadata, exact-version resolution, external compilation, linking, and execution on Raspberry Pi arm64.

### Privilege note

The recorded full validation was launched with `sudo bash` because the original script changed its working directory to the installed executable directory, `/bin`. Test 14 and other file-I/O checks create relative temporary files, so an ordinary user could not write there and four tests failed.

This was a validation-script working-directory defect, not a CCSDSPack runtime or package defect. Running as root does not change packet serialization, parsing, CRC, CLI integration, CMake discovery, or external-consumer results, so the recorded PASS remains valid.

The script has been corrected to execute `CCSDSPack_tester` from a writable temporary directory. Future validation runs must launch the script as a normal user; the script invokes `sudo` only for `dpkg -i`.

## Reproducing the Raspberry Pi validation

### 1. Check out the candidate

```bash
git clone https://github.com/ExoSpaceLabs/CCSDSPack.git
cd CCSDSPack
git checkout <candidate-commit>
```

Confirm that the host is native arm64:

```bash
uname -m
```

Expected:

```text
aarch64
```

### 2. Obtain or build the package

Use the arm64 DEB produced by CI, or build it natively on the Raspberry Pi:

```bash
./package.sh -p DEB
```

Building the package does not require root privileges. The resulting package is placed under `packages/`.

```bash
ARM64_DEB="$(find ./packages -type f -name '*arm64*.deb' -print -quit)"
```

### 3. Record package metadata

```bash
dpkg-deb -f "$ARM64_DEB" Package
dpkg-deb -f "$ARM64_DEB" Version
dpkg-deb -f "$ARM64_DEB" Architecture
sha256sum "$ARM64_DEB"
```

Required metadata:

```text
Package: ccsdspack
Version: 1.2.0
Architecture: arm64
```

### 4. Run the complete validation as a normal user

```bash
bash test/package_tester/aarch64_validate.sh "$ARM64_DEB" \
  2>&1 | tee ~/ccsdspack-aarch64-validation.log
```

Do not prefix the command with `sudo`. The script uses `sudo` only to install the package.

The required final line is:

```text
CCSDSPACK_AARCH64_TEST:PASS
```

### 5. Preserve release evidence

Record:

```bash
uname -a
cat /etc/os-release
git rev-parse HEAD
dpkg-deb -f "$ARM64_DEB" Package Version Architecture
sha256sum "$ARM64_DEB"
```

Preserve the complete validation log with the release records.

## Acceptance boundary

This result establishes that the v1.2.0 package and installed interfaces execute successfully on the tested Raspberry Pi 5 arm64 environment. It does not claim qualification of every Raspberry Pi model, Linux distribution, kernel, compiler, timing condition, long-duration workload, or future release artifact.

The packages generated by the final `v1.2.0` tag must still be downloaded and checked as part of publication verification.

## STM32 next step

Use the board-independent validation core in:

```text
test/package_tester/stm32h7xx/CM7/Inc/ccsdspack_mcu_test.h
```

from a native STM32H755 CM7 project following [H755_INTEGRATION.md](../test/package_tester/stm32h7xx/H755_INTEGRATION.md). Record the board revision, compiler version, candidate commit, archive hash, memory usage, flash/reset result, absence of runtime faults, and final UART marker:

```text
CCSDSPACK_MCU_TEST:PASS
```
