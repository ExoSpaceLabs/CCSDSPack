<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v1.2 hardware validation

[Documentation index](README.md) | [Packages](PACKAGES.md) | [v1.2 release notes](releases/v1.2.0.md)

This page records the release hardware evidence for CCSDSPack v1.2.0 and provides the repeatable validation procedures. Hardware execution complements the Linux and Windows CI evidence; it does not extend the compliance claim beyond the CCSDS Space Packet PDU profile described in [COMPLIANCE.md](../COMPLIANCE.md) and [CCSDS_COMPLIANCE.md](../CCSDS_COMPLIANCE.md).

## Status

| Target | Status | Required marker |
|---|---|---|
| Raspberry Pi 5, native arm64 Linux | **PASS** | `CCSDSPACK_AARCH64_TEST:PASS` |
| STM32H755ZITx / NUCLEO-H755ZI-Q CM7 | **PASS** | `CCSDSPACK_MCU_TEST:PASS` |

Both v1.2 hardware-validation gates are accepted. The validated `develop` state was promoted to `main` through pull request #107. Tag publication and verification of the generated packages and container images remain separate release controls.

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

The corrected validation harness was also rerun as an ordinary user. It elevated only `dpkg -i`, copied the installed tester fixtures to a writable temporary directory, and produced the same final PASS marker.

The test covers native package installation, dynamic-library loading, packet regression and conformance behaviour, installed command-line tools, CMake package metadata, exact-version resolution, external compilation, linking, and execution on Raspberry Pi arm64.

### Installed test resources and privileges

The package installs `CCSDSPack_tester` together with a sibling `test_resources` directory under the executable installation directory. These files are regression-test fixtures only. They are not required by `libccsdspack`, the encoder, decoder, validator, or applications that link CCSDSPack.

The tester intentionally uses relative paths such as `test_resources/core_packet.bin` and `test_resources/myPackets.bin`. It reads committed fixtures from that directory and creates temporary round-trip files there during the file-I/O tests.

The original full validation was launched with `sudo bash` because the old script ran the installed tester from `/bin`. The installed `/bin/test_resources` directory is root-owned, so an ordinary user could read the fixtures but could not create the temporary test files. Four tests consequently failed without elevation.

This was a validation-harness working-directory and permissions issue, not a CCSDSPack library or package failure. The root-run PASS remains valid because elevation did not change packet serialization, parsing, CRC behaviour, CLI integration, CMake discovery, or external-consumer execution.

The corrected script copies the installed `test_resources` fixtures into a writable temporary work directory while preserving the expected relative layout, runs `CCSDSPack_tester` there as the invoking user, and removes the copy afterward. Package installation still requires root permissions; the script invokes `sudo dpkg -i` for that operation only.

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

### 4. Run the complete validation

```bash
bash test/package_tester/aarch64_validate.sh "$ARM64_DEB" \
  2>&1 | tee ~/ccsdspack-aarch64-validation.log
```

Launch the script as a normal user. It requests elevation for `dpkg -i`, because system package installation requires root permissions, then runs all validation tests unprivileged from writable locations.

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

## STM32H755 CM7 validation record

Validation date: **2026-08-02**

### Platform and execution path

- Board: `NUCLEO-H755ZI-Q`
- Target core: Cortex-M7
- Support core: Cortex-M4 image used for the committed dual-core startup sequence
- Validation application: committed STM32H745/H755 shared project and HAL sources
- Serial interface: ST-LINK V3 Virtual COM Port
- UART: 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control
- Committed validation baseline: `develop` state represented by commit `398231bfc429e892b923f97b88d0a7a4674417f5`

The committed project retains STM32H745 target metadata and therefore prints an `STM32H745` banner. The validation procedure explicitly accepts that banner when the images execute on the NUCLEO-H755ZI-Q because the selected H745/H755 project sources and startup path are shared for this test.

### Results

CM4 was programmed before CM7, the board was reset, and the deterministic validation suite completed successfully twice. The captured UART output was:

```text
CCSDSPack STM32H745 CM7 hardware validation
Running packet generation, parsing, CRC, Manager, Validator, PVN, and Idle tests...
CCSDSPACK_MCU_TEST:PASS
Reset the board to run the validation again.
```

The same PASS sequence was observed again after a second reset.

The hardware test exercises:

- the C++17 `CCSDS_MCU` consumer profile;
- target-side allocation and shared ownership used by the test;
- custom secondary-header registration;
- packet generation and exact CRC16 vector comparison;
- `Manager` template setup and sequence-count advancement;
- bounded parsing and consumed-byte reporting;
- decoded header, secondary-header, application-data, and CRC fields;
- `Validator` acceptance of the valid packet;
- CRC-disabled generation and parsing;
- rejection of non-zero Packet Version Number serialization;
- invalid and valid Idle Packet behaviour.

No `CCSDSPACK_MCU_TEST:FAIL:<code>` or `CCSDSPACK_MCU_TEST:HAL_FAILURE` marker appeared in the supplied capture.

The exact build, middleware installation, CubeIDE configuration, dual-core flash order, UART setup, failure mapping, and acceptance criteria are recorded in [V1_2_STM32_VALIDATION_STEPS.md](V1_2_STM32_VALIDATION_STEPS.md).

## Acceptance boundary

These results establish that the v1.2.0 package and supported interfaces execute successfully on the tested Raspberry Pi 5 arm64 environment and that the deterministic MCU suite executes successfully on the tested NUCLEO-H755ZI-Q CM7 configuration.

They do not claim qualification of every Raspberry Pi model, Linux distribution, kernel, compiler, STM32H7 variant, board revision, timing condition, long-duration workload, radiation environment, or future release artifact.

Packages and container images produced from the final `v1.2.0` tag must be downloaded or pulled and checked against the tagged commit as part of publication verification.
