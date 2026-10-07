<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack v2 hardware validation

[Documentation index](README.md) | [Packages](PACKAGES.md) | [Release acceptance](../V2_TRANSITION_ACCEPTANCE_LIST.md)

This page records physical-target and native-target release evidence for CCSDSPack v2 releases. Hardware execution complements hosted CI and package/cross-build evidence; it does not extend the documented compliance scope beyond the supported CCSDS Space Packet PDU, PUS, CUC, and mission-tailoring profiles.


## v2.1.0 validation evidence

v2.1.0 changes the implementation architecture substantially while preserving the v2 C++ API. Fresh physical/native execution was therefore required and completed before tagging.

| Target | v2.1 status | Required marker |
|---|---|---|
| Raspberry Pi 5, native arm64 Linux | **PASS** | `CCSDSPACK_AARCH64_TEST:PASS` |
| NUCLEO-H755ZI-Q, Cortex-M7 | **PASS** | `CCSDSPACK_HARDWARE_TEST:PASS` |

The exact validated source commits and generated package/library hashes are recorded below. The final qualified `main` commit is `93f43b6fc4b9e3395dd8273ed63e182f42a8f3b4`.

### Raspberry Pi 5 / arm64 rerun

For a future rerun from a clean native arm64 checkout:

```bash
git checkout main
git pull --ff-only
bash test/package_tester/run_aarch64_validation.sh
```

The top-level runner records the board, architecture, OS, kernel, compiler,
CMake, Python, branch, source SHA, and source version; removes previous native
build/package output; builds the DEB from scratch; validates package name,
version, architecture, and SHA-256; then invokes `aarch64_validate.sh` for the
installed regression suite, CLI integration, installed-package consumer, and
shared hardware-acceptance body. The complete run is saved by default to
`~/ccsdspack-v<version>-aarch64-validation.log`.

The runner deliberately does not install missing host dependencies or update
the source checkout. Those are preparation steps, not part of release
qualification.

Acceptance requires:

```text
CCSDSPACK_HARDWARE_TEST:PASS
CCSDSPACK_AARCH64_TEST:PASS
CCSDSPACK_AARCH64_RUNNER:PASS
```

`aarch64_validate.sh <package.deb>` remains available as the lower-level
package validator when an already-built ARM64 package must be qualified
directly.

### NUCLEO-H755ZI-Q / Cortex-M7 rerun

The v2.1 physical harness is a standalone CMake/OpenOCD application. It links
the generated CCSDSPack MCU package with the pinned Device Abstraction Stack
(DAS), which supplies Cortex-M startup/vector/linker support, board clocking and
the ST-LINK VCP UART. It does **not** use STM32CubeIDE, STM32 HAL, the Nucleo
BSP, generated vendor makefiles, or a CM4 companion project.

For a future physical rerun, generate the MCU package from the exact source commit being qualified:

```bash
./package.sh \
  -t cmake/toolchains/arm-none-eabi.cmake \
  -p MCU \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
```

Execute the complete build/flash/UART acceptance flow with:

```bash
bash test/package_tester/stm32h7xx/run_h755_validation.sh \
  --package packages/ccsdspack-v2.1.0-Generic-arm.tar.gz \
  --source-sha <candidate-sha>
```

The runner pins DAS and the minimal STM32 CMSIS source revision, builds the
standalone Cortex-M7 ELF, records its SHA-256 and `text/data/bss`, programs and
verifies it through OpenOCD, captures the ST-LINK VCP at 115200 8N1, and fails
unless the target emits:

```text
CCSDSPACK_HARDWARE_TEST:PASS
```

The UART transcript also contains candidate/package/DAS/compiler/runtime
identity, per-section BEGIN/PASS markers, symbolic failure names, and explicit
HardFault/MemManage/BusFault/UsageFault markers.

For future releases, record the exact candidate SHA, MCU package SHA-256,
linked ELF SHA-256, compiler version, final ELF text/data/bss, complete UART
transcript, and OpenOCD program/verify log before release promotion. See
`test/package_tester/stm32h7xx/H755_INTEGRATION.md` for the full procedure.

### v2.1.0 validation evidence

Validation date: **2026-10-04**

#### Raspberry Pi 5 / native arm64

Platform and toolchain:

- Board: Raspberry Pi 5 Model B Rev 1.0
- Architecture: `aarch64`
- Operating system: Debian GNU/Linux 13 (`trixie`)
- Kernel: `6.18.34+rpt-rpi-2712`
- GCC/G++: 14.2.0
- CMake: 3.31.6
- Python: 3.13.5

Candidate and package identity:

- Branch: `develop`
- Source commit: `9f6fcbfda263d1737f50833a5af14c2b6f104799`
- Source version: `2.1.0`
- Package: `ccsdspack-v2.1.0-Linux-arm64.deb`
- Package architecture: `arm64`
- Package SHA-256: `41354b83ba50d73f1804969ba72628e2cdc8e2aa2d6551f207b79ccb0ec1517f`

The one-command native runner rebuilt the DEB from a clean checkout, verified
package identity, installed it, and completed the package-level validation.
Results:

- regression/conformance suite: **134 passed, 0 failed**;
- CLI integration: **PASS**;
- external installed-package CMake consumer: **1 passed, 0 failed**;
- shared hardware acceptance: `CCSDSPACK_HARDWARE_TEST:PASS`;
- native arm64 acceptance: `CCSDSPACK_AARCH64_TEST:PASS`.

The run log is written by the runner to
`~/ccsdspack-v2.1.0-aarch64-validation.log`.

#### NUCLEO-H755ZI-Q / Cortex-M7

Platform and runtime identity:

- Physical board: NUCLEO-H755ZI-Q
- Core: Cortex-M7
- Transport: DAS UART through ST-LINK VCP
- UART: 115200 8N1
- Core clock: 400 MHz
- Compiler: GNU Arm Embedded 10.3.1
- C++ standard: C++17

Candidate and dependency identity:

- CCSDSPack source commit: `08cc587b41217e5d4ebc9dd31407c51fdb7de153`
- MCU package SHA-256: `d8f920eab1528b4323670a52667fced2e9a3934ea7b352f66b140fdf399e6d54`
- linked `libccsdspack.a` SHA-256: `4936e9e5fb1b29141797b993d983d7c5515c85fd3b60fc9188184a8b6d911ee5`
- DAS revision: `4b768ef86b43652c94cc91b1c77e247fa37ebd8a`

The standalone DAS/OpenOCD validation programmed and verified the target, then
passed every runtime acceptance section:

- generic Packet/Manager;
- raw-buffer parsing;
- structured Validator;
- raw Manager reassembly;
- PUS-C telecommand;
- PEC-none operation;
- Packet Version Number rejection;
- Idle Packet policy.

Final runtime result:

```text
TEST:PASS:all
RESULT_CODE:0
RESULT_NAME:Pass
HEAP_CAPACITY_BYTES:507696
HEAP_PEAK_BYTES:2996
CCSDSPACK_HARDWARE_TEST:PASS
CCSDSPACK_HARDWARE_TEST:END
```

The OpenOCD 0.11 target script still emits non-fatal STM32H7 DBGMCU
`mem2array` examine warnings on this host, but flash programming and
verification complete successfully and the firmware executes the full
acceptance suite.

### Final release integration note

The final qualified `main` commit for v2.1.0 is
`93f43b6fc4b9e3395dd8273ed63e182f42a8f3b4`. Comparison from both physical/native
validation source commits to that final release line shows no changes under
`src/`, `inc/`, the root `CMakeLists.txt`, or `package.sh`; subsequent changes
are confined to validation tooling, performance harnesses, CI, and documentation.
The recorded Pi/H755 executions therefore qualify the same library implementation
that is being released.

### Pre-hardware footprint evidence

A matched Cortex-M7 compile/link comparison against v2.0 `main` uses the same v2.0 public hardware probe for both implementations. With section garbage collection, retained text is:

- v2.0 main: **36,238 bytes**;
- v2.1 candidate: **40,166 bytes**;
- delta: **+3,928 bytes (+10.8%)**.

This is compile/link evidence only. The physical STM32 execution record above remains authoritative for the v2.1 hardware qualification.

---

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

## Historical v2.0 release status

The v2.0.0 native arm64 and physical Cortex-M7 gates recorded above were subsequently incorporated into the published v2.0.0 release. The original source/package/library identities are retained here as historical evidence; they are not open release gates for v2.1.0.
