<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v1.2 hardware validation

[Documentation index](README.md) | [Compliance matrix](../CCSDS_COMPLIANCE.md) | [Packages](PACKAGES.md) | [v1.2 release notes](releases/v1.2.0.md)

This page records the release-candidate hardware evidence for CCSDSPack v1.2.0 and provides repeatable validation references. Hardware execution complements the Linux and Windows CI evidence; it does not extend the compliance claim beyond the CCSDS Space Packet PDU profile described in [CCSDS_COMPLIANCE.md](../CCSDS_COMPLIANCE.md).

## Status

| Target | Status | Required marker |
|---|---|---|
| Raspberry Pi 5, native arm64 Linux | **PASS** | `CCSDSPACK_AARCH64_TEST:PASS` |
| STM32H755ZITx / NUCLEO-H755ZI-Q CM7 | **PASS** | `CCSDSPACK_MCU_TEST:PASS` |

Both v1.2.0 hardware-validation gates are complete. Remaining release work is promotion to `main`, final `main` CI, tagging, publication, and artifact verification.

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

### Installed test resources and privileges

The package installs `CCSDSPack_tester` together with a sibling `test_resources` directory under the executable installation directory. These files are regression-test fixtures only. They are not required by `libccsdspack`, the encoder, decoder, validator, or applications that link CCSDSPack.

The tester intentionally uses relative paths such as `test_resources/core_packet.bin` and `test_resources/myPackets.bin`. It reads committed fixtures from that directory and creates temporary round-trip files there during the file-I/O tests.

The recorded validation was launched with `sudo bash` because the original script ran the installed tester from `/bin`. The installed `/bin/test_resources` directory is root-owned, so an ordinary user could read the fixtures but could not create the temporary test files. Four tests consequently failed without elevation.

This was a validation-harness working-directory and permissions issue, not a CCSDSPack library or package failure. The root-run PASS remains valid because elevation did not change packet serialization, parsing, CRC behaviour, CLI integration, CMake discovery, or external-consumer execution.

The validation script now copies the installed `test_resources` fixtures into a writable temporary work directory while preserving the expected relative layout, runs `CCSDSPack_tester` there as the invoking user, and removes the copy afterward. Package installation still requires root permissions; the script invokes `sudo dpkg -i` for that operation only.

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

## Raspberry Pi acceptance boundary

This result establishes that the v1.2.0 package and installed interfaces execute successfully on the tested Raspberry Pi 5 arm64 environment. It does not claim qualification of every Raspberry Pi model, Linux distribution, kernel, compiler, timing condition, long-duration workload, or future release artifact.

The packages generated by the final `v1.2.0` tag must still be downloaded and checked as part of publication verification.

## STM32H755 Cortex-M7 validation record

Validation date: **2026-08-02**

### Platform and terminal

- Board: NUCLEO-H755ZI-Q
- Tested core: Cortex-M7
- Validation project: retained dual-core H745/H755 STM32CubeIDE project documented in [V1_2_STM32_VALIDATION_STEPS.md](V1_2_STM32_VALIDATION_STEPS.md)
- Shared deterministic test core: `test/package_tester/stm32h7xx/CM7/Inc/ccsdspack_mcu_test.h`
- Serial interface: `/dev/serial/by-id/usb-STMicroelectronics_STLINK-V3_004C003E3234510433353533-if02`
- Terminal: picocom 3.1
- UART: 115200 baud, 8 data bits, no parity, one stop bit, no flow control

The executable banner retains the text `STM32H745 CM7 hardware validation` because the accepted project metadata and harness retain the H745 label while running on the H755 validation board. The validation procedure documents this H745/H755 shared-project mapping.

### Results

The firmware built, linked, programmed, started, and executed the deterministic CCSDSPack MCU suite successfully. The captured UART sequence was:

```text
CCSDSPack STM32H745 CM7 hardware validation
Running packet generation, parsing, CRC, Manager, Validator, PVN, and Idle tests...
CCSDSPACK_MCU_TEST:PASS
Reset the board to run the validation again.
```

After reset, the complete sequence ran successfully a second time and again produced:

```text
CCSDSPACK_MCU_TEST:PASS
```

This demonstrates successful operation of the tested static archive, public MCU headers, application ABI, startup path, dual-core boot sequence, C++ runtime, heap-backed STL use, packet generation, exact CRC16 and CRC-free vectors, Manager sequence handling, bounded parsing, Validator behavior, Packet Version Number rejection, and Idle Packet rules on the target.

No HardFault, MemManage, BusFault, UsageFault, or allocation failure was observed during either run.

The UART excerpt does not itself repeat the compiler version, package SHA-256, linker map, or memory-usage figures requested by the detailed procedure. Those values remain useful reproducibility metadata, but the repeated deterministic PASS closes the v1.2 STM32 runtime gate.

## Reproducing the STM32 validation

Follow [V1_2_STM32_VALIDATION_STEPS.md](V1_2_STM32_VALIDATION_STEPS.md). The procedure covers:

- building the v1.2 Cortex-M7 archive with matching hard-float ABI flags;
- installing the complete public header set and `libccsdspack.a`;
- configuring `CCSDS_MCU`, C++17, exceptions disabled, and RTTI disabled;
- building and programming both CM4 and CM7 images;
- preserving the accepted H745/H755 project mapping;
- opening the ST-LINK virtual COM interface at 115200 8N1;
- capturing the deterministic PASS marker.

## STM32 acceptance boundary

This result establishes that the deterministic v1.2.0 MCU validation suite executes successfully on the tested NUCLEO-H755ZI-Q Cortex-M7 configuration. It does not qualify every STM32H7 device, board revision, compiler release, linker layout, heap size, clock configuration, or mission workload.

## Release implication

The Raspberry Pi arm64 and STM32 Cortex-M7 hardware gates are complete. The remaining v1.2.0 release sequence is:

1. merge the validated `develop` state into `main`;
2. confirm Linux and Windows CI at the selected `main` commit;
3. create tag `v1.2.0`;
4. publish and verify packages, the GitHub Release, and GHCR images;
5. close release issue #95 and parent issue #46.
