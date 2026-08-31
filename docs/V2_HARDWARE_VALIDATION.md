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
| NUCLEO-H755ZI-Q, Cortex-M7 | **PASS** | `CCSDSPACK_HARDWARE_TEST:PASS` |

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

This run satisfies the v2.0.0 release gate requiring fresh native arm64 installed-package/API execution against the v2 hardware-acceptance baseline.

## NUCLEO-H755ZI-Q Cortex-M7 validation record

Validation date: **2026-08-31**

### Platform and compatibility basis

- Physical board: **NUCLEO-H755ZI-Q**
- Execution core: Cortex-M7
- ST-Link V3 virtual COM interface used for the runtime result
- UART: 115200 baud, 8 data bits, no parity, 1 stop bit

The STM32CubeH7 distribution documents that all projects under `Projects/NUCLEO-H745ZI-Q` are fully compatible with the NUCLEO-H755ZI-Q board. The release validation therefore uses the ST-provided H745/H755-compatible project configuration rather than treating H745-generated project names as evidence of a different physical target. H755-only cryptographic functionality is outside CCSDSPack's scope.

Official ST compatibility note:

`https://github.com/STMicroelectronics/STM32CubeH7/blob/master/Projects/NUCLEO-H755ZI-Q/readme.txt`

### Candidate and package

- Release-candidate source commit: `3cd3ddd67be09b7ad7f3d52360b2f3c858b1fee3`
- Compiler: `arm-none-eabi-g++ 10.3.1 20210621 (release)`
- Package: `ccsdspack-v2.0.0-Generic-arm.tar.gz`
- Package SHA-256: `dec18a45f6d34198fd621e39b2669893f0155134a0106782c4574fb16393223b`
- Installed `libccsdspack.a` SHA-256: `e53f37b37e87a0e841d33475102594d56215f1502c0137016b1a453f744994eb`

The package was rebuilt from a clean detached worktree using the explicit MCU flag path after PR #130 fixed `-m/--mcu-flags` parsing:

```bash
./package.sh \
  -t cmake/toolchains/arm-none-eabi.cmake \
  -p MCU \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
```

The resulting archive was `elf32-littlearm`, architecture `armv7e-m`. The installed middleware archive hash was checked against the archive extracted from the package and matched exactly.

### Application build

The CM7 application compiled as C++17 with `CCSDS_MCU`, `-fno-exceptions`, `-fno-rtti`, `-fno-use-cxa-atexit`, Cortex-M7, `fpv5-d16`, hard-float ABI, and linked the installed `libccsdspack.a`.

Final linked ELF size:

```text
   text    data     bss     dec     hex
 100124     112    2056  102292   18f94
```

### Runtime result

After flashing the CM7 image onto the physical NUCLEO-H755ZI-Q and opening the ST-Link virtual COM port, the board reported:

```text
CCSDSPack CM7 hardware validation
Running shared Packet, PEC, PUS, Validator, raw-buffer, Manager, PVN, and Idle acceptance...
CCSDSPACK_HARDWARE_TEST:PASS
Reset the board to run the validation again.
```

The shared acceptance body therefore completed generic Packet/CRC, Manager sequence behavior, packet-level PEC CRC16/None, structured Validator checks, PUS-C telecommand construction/parsing/validation, bounded raw-buffer framing/parsing, truncation rejection, raw Manager reconstruction, Packet Version Number rejection, and Idle Packet constraints on the physical Cortex-M7 target.

No HardFault, MemManage, BusFault, allocation-failure, or test-failure marker was observed before the PASS result.

### Acceptance

This run satisfies the v2.0.0 physical Cortex-M7 execution gate for the tested NUCLEO-H755ZI-Q board and exact source/package/library identities recorded above.

## Release status

Fresh native arm64 and physical Cortex-M7 execution gates are complete. Remaining v2.0.0 gates are release/publication control: final evidence/release-note approval, `develop -> main`, final `main` CI, tag creation, and verification of the tag-produced GitHub Release, packages, and GHCR images.
