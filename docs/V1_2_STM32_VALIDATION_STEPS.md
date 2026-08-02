# CCSDSPack v1.2.0 STM32H755 Validation Procedure

## Purpose

Close the remaining CCSDSPack v1.2.0 hardware release gate by executing the existing deterministic Cortex-M7 validation suite on the NUCLEO-H755ZI-Q.

This validation is limited to the v1.2 CCSDS Space Packet implementation. ECSS PUS revision redesign and integrated PUS validation belong to v2.0.0 and are not part of this run.

## Target and repository baseline

- Repository: `ExoSpaceLabs/CCSDSPack`
- Branch: `develop`
- Board: `NUCLEO-H755ZI-Q`
- Core: Cortex-M7
- Existing project: `test/package_tester/stm32h7xx`
- CubeIDE CM4 project: `test/package_tester/stm32h7xx/STM32CubeIDE/CM4`
- CubeIDE CM7 project: `test/package_tester/stm32h7xx/STM32CubeIDE/CM7`
- Shared validation core: `test/package_tester/stm32h7xx/CM7/Inc/ccsdspack_mcu_test.h`
- Entry point: `test/package_tester/stm32h7xx/CM7/Src/main.cpp`

For this validation, retain the committed STM32H745 project metadata and sources when using the H755 board. The H745/H755 naming discrepancy is accepted because the relevant ST project sources are shared/mapped accordingly. Do not regenerate a separate H755 project merely to rename the target.

## 1. Capture the validation baseline

From the repository root:

```bash
git switch develop
git pull --ff-only

git status --short
git rev-parse HEAD
arm-none-eabi-g++ --version | head -n 1
```

Acceptance:

- Working tree is clean.
- The exact `develop` commit SHA is recorded.
- The ARM GNU toolchain version is recorded.

## 2. Build the v1.2 MCU package

```bash
./package.sh \
  -t cmake/toolchains/arm-none-eabi.cmake \
  -p MCU \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
```

The command must:

- configure CCSDSPack `1.2.0` with `CCSDSPACK_BUILD_MCU=ON`;
- compile the static archive with `CCSDS_MCU`;
- compile the shared HAL-independent STM32 test core;
- perform the relocatable link probe;
- generate an MCU TGZ package under `packages/`.

Record the package and archive evidence:

```bash
ls -lh packages/ccsdspack-v1.2.0-*.tar.gz
sha256sum packages/ccsdspack-v1.2.0-*.tar.gz
find build lib -name libccsdspack.a -type f -print
```

Acceptance:

- `libccsdspack.a` exists.
- The MCU TGZ package exists.
- The compile/link probe completes without errors.
- Package SHA-256 is recorded.

## 3. Install the current headers and library into the middleware location

The `Middlewares` directory is the dependency installation location selected for the
STM32CubeIDE/HAL workspace. It does **not** have to be inside the CCSDSPack repository.
For the current workstation, the installed dependency root is:

```text
/home/dev/Middlewares/Third_Party/CCSDSPack/
```

Use this layout:

```text
/home/dev/Middlewares/Third_Party/CCSDSPack/
├── includes/
│   ├── CCSDSPack.h
│   ├── CCSDSDataField.h
│   ├── CCSDSHeader.h
│   ├── CCSDSManager.h
│   ├── CCSDSPacket.h
│   ├── CCSDSResult.h
│   ├── CCSDSSecondaryHeaderAbstract.h
│   ├── CCSDSUtils.h
│   ├── CCSDSValidator.h
│   ├── PusServices.h
│   └── all other installed public headers
└── lib/
    └── libccsdspack.a
```

The complete public header set is required. Copying only `CCSDSPack.h` is insufficient
because it includes the remaining public headers.

Inspect and extract the generated MCU package:

```bash
PACKAGE=$(ls packages/ccsdspack-v1.2.0-*.tar.gz | head -n 1)
rm -rf /tmp/ccsdspack-v1.2-mcu
mkdir -p /tmp/ccsdspack-v1.2-mcu

tar -xzf "$PACKAGE" -C /tmp/ccsdspack-v1.2-mcu

MIDDLEWARE_ROOT=/home/dev/Middlewares/Third_Party/CCSDSPack
mkdir -p "$MIDDLEWARE_ROOT/includes" "$MIDDLEWARE_ROOT/lib"

find /tmp/ccsdspack-v1.2-mcu -type f -path '*/include/*.h' \
  -exec cp -v {} "$MIDDLEWARE_ROOT/includes/" \;

cp -v "$(find /tmp/ccsdspack-v1.2-mcu -type f -name libccsdspack.a -print -quit)" \
  "$MIDDLEWARE_ROOT/lib/libccsdspack.a"
```

Verify the installation:

```bash
find /home/dev/Middlewares/Third_Party/CCSDSPack/includes \
  -maxdepth 1 -type f -name '*.h' -printf '%f\n' | sort

ls -lh /home/dev/Middlewares/Third_Party/CCSDSPack/lib/libccsdspack.a
```

Acceptance:

- The middleware installation contains all public headers from the current `develop` package.
- `libccsdspack.a` comes from the same package and commit as those headers.
- No older header set remains mixed with the v1.2 archive.

## 4. Open and configure the existing CubeIDE CM7 project

Import/open:

```text
test/package_tester/stm32h7xx/STM32CubeIDE/CM7
```

Do not create a new project. Do not replace the committed H745/H755 shared project
sources solely because CubeIDE displays `STM32H745ZITx` or `NUCLEO-H745ZI-Q`.

Configure **both Debug and Release**, preferably by selecting **All configurations**
in the CubeIDE project properties.

### Required C++ preprocessor symbol

In STM32CubeIDE:

```text
Project
  → Properties
  → C/C++ Build
  → Settings
  → MCU G++ Compiler
  → Preprocessor
  → Defined symbols (-D)
```

Add:

```text
CCSDS_MCU
```

This is required for every C++ translation unit consuming the MCU headers. After a
clean rebuild, the `arm-none-eabi-g++` command compiling `main.cpp` must contain:

```text
-DCCSDS_MCU
```

If it is absent, the validation header deliberately stops compilation with:

```text
#error "Define CCSDS_MCU when compiling the STM32 validation application"
```

Without `CCSDS_MCU`, host-only configuration APIs remain visible. In particular,
`SecondaryHeaderAbstract::loadFromConfig()` remains pure virtual, which makes
`CustomSecondaryHeader` appear abstract and produces the misleading secondary error:

```text
invalid new-expression of abstract class type
```

Do not add a dummy `loadFromConfig()` implementation to the MCU validation class as a
workaround. Fix the missing CubeIDE preprocessor symbol; both errors then disappear.

### C++ compiler configuration

Required settings:

- C++ standard: GNU C++17 or C++17
- Preprocessor symbol: `CCSDS_MCU`
- Additional flags:

```text
-fno-exceptions
-fno-rtti
-fno-use-cxa-atexit
```

The target ABI must match the archive:

```text
-mcpu=cortex-m7
-mthumb
-mfpu=fpv5-d16
-mfloat-abi=hard
```

### Include path

Use the actual middleware installation selected on the workstation:

```text
/home/dev/Middlewares/Third_Party/CCSDSPack/includes
```

Configure it under:

```text
MCU G++ Compiler → Include paths (-I)
```

An absolute path is acceptable when it intentionally points to the local middleware
installation. It is stale only when it points to a different workstation or an older
package.

### Linker configuration

Library search path:

```text
/home/dev/Middlewares/Third_Party/CCSDSPack/lib
```

Library name:

```text
ccsdspack
```

This must resolve to:

```text
libccsdspack.a
```

Do not use the obsolete library name `ccsdspack_mcu`.

### Rebuild after changing settings

Run:

```text
Project → Clean...
Project → Build Project
```

Inspect the emitted `arm-none-eabi-g++` line for `main.cpp`. It must contain all of:

```text
-DCCSDS_MCU
-std=gnu++17
-fno-exceptions
-fno-rtti
-mcpu=cortex-m7
-mfpu=fpv5-d16
-mfloat-abi=hard
-mthumb
-I/home/dev/Middlewares/Third_Party/CCSDSPack/includes
```

## 5. Confirm the validation harness is active

The CM7 entry point must include:

```cpp
#include "ccsdspack_mcu_test.h"
```

After HAL, clock, cache/MPU, UART, LED, and runtime initialization, it must execute:

```cpp
const int result = CCSDSPackMcuTest::run();
```

The committed `CM7/Src/main.cpp` already performs this automatically after reset.

The test validates:

- C++17/`CCSDS_MCU` consumer operation;
- STL allocation and shared ownership on the configured newlib heap;
- custom secondary-header registration;
- Manager template setup and sequence-count advancement;
- exact CRC16 wire vector;
- bounded parsing and consumed-byte reporting;
- decoded header, secondary-header, application-data, and CRC fields;
- Validator behavior;
- CRC-disabled generation and parsing;
- rejection of non-zero Packet Version Number serialization;
- invalid and valid Idle Packet behavior.

## 6. Build and inspect the CM7 image

Build the CM7 project in STM32CubeIDE.

Acceptance:

- All C and C++ sources compile.
- `CCSDS_MCU` is visible to the application translation units.
- The final link resolves `libccsdspack.a` without ABI or symbol errors.
- No exception/RTTI runtime dependency is unexpectedly introduced.
- No stale absolute path is used.

Record:

- final ELF path;
- flash usage;
- RAM usage;
- configured minimum heap and stack;
- linker map file, when available.

A successful link is necessary but is not the hardware release gate. Silicon execution remains mandatory because humanity has repeatedly demonstrated that “it linked” is not equivalent to “it works.”

## 7. Build and flash both STM32 cores

The STM32H745/H755 validation project is dual-core. The CM7 startup waits for the
Cortex-M4/D2 domain to enter STOP mode before HAL, UART, LEDs, and the CCSDSPack test
are initialized. Flashing only CM7 can therefore produce a successfully programmed
board with no UART output and no useful LED indication.

### 7.1 Build the CM4 support image

Import/open:

```text
test/package_tester/stm32h7xx/STM32CubeIDE/CM4
```

Build the `UART_WakeUpFromStopUsingFIFO_CM4` project. Its support firmware activates
the hardware semaphore notification and places the D2 domain into STOP mode so the
CM7 startup sequence can continue.

Record the generated CM4 ELF path, normally similar to:

```text
test/package_tester/stm32h7xx/STM32CubeIDE/CM4/Debug/UART_WakeUpFromStopUsingFIFO_CM4.elf
```

### 7.2 Build the CM7 validation image

Build the `UART_WakeUpFromStopUsingFIFO_CM7` project and record the generated ELF,
normally similar to:

```text
test/package_tester/stm32h7xx/STM32CubeIDE/CM7/Debug/UART_WakeUpFromStopUsingFIFO_CM7.elf
```

### 7.3 Flash order with STM32CubeProgrammer

1. Connect the NUCLEO-H755ZI-Q over the on-board ST-LINK USB connector.
2. In STM32CubeProgrammer, select `ST-LINK` and connect to the target.
3. Open **Erasing & Programming**.
4. Select and program the CM4 ELF first.
5. Do **not** perform a full-chip erase after programming CM4.
6. Select and program the CM7 ELF.
7. Enable **Run after programming** when available, or press the physical reset button
   after both images have been programmed.
8. Keep the serial terminal open before resetting so the one-shot startup log is not
   missed.

The SWD/ST-LINK connection speed is unrelated to the UART baud rate. Some
STM32CubeProgrammer connection modes do not expose an editable SWD frequency; this
does not prevent programming or change the `115200` UART configuration.

The same order can be used from STM32CubeIDE by launching/programming the CM4 project
first and then the CM7 project. A build operation alone does not write either image to
the MCU, because apparently even embedded tools require this distinction to be stated
with courtroom precision.

Acceptance:

- Both CM4 and CM7 images program successfully.
- A board reset starts the CM4 support firmware and allows CM7 startup to continue.
- No full-chip erase removes the CM4 image between the two programming operations.
- No HardFault, MemManage, BusFault, UsageFault, or allocation failure occurs.

## 8. Install picocom and capture UART evidence

### 8.1 Install picocom

On Ubuntu or Debian:

```bash
sudo apt update
sudo apt install -y picocom
```

Confirm installation:

```bash
picocom --version
```

If the user account cannot open the serial device, add it to the `dialout` group:

```bash
sudo usermod -aG dialout "$USER"
```

Log out and back in after changing group membership. Do not run `picocom` permanently
as root merely to avoid fixing device permissions.

### 8.2 Identify the ST-LINK Virtual COM Port

Prefer the stable `/dev/serial/by-id/` path instead of assuming that the device will
always be `/dev/ttyACM0`:

```bash
ls -l /dev/serial/by-id/
```

Example observed device:

```text
usb-STMicroelectronics_STLINK-V3_004C003E3234510433353533-if02 -> ../../ttyACM0
```

### 8.3 Open the terminal

UART configuration:

```text
115200 baud
8 data bits
no parity
1 stop bit
no flow control
```

Recommended command:

```bash
picocom -b 115200 -d 8 -p n -f n \
  /dev/serial/by-id/usb-STMicroelectronics_STLINK-V3_004C003E3234510433353533-if02
```

A generic device path may also be used when confirmed:

```bash
picocom -b 115200 -d 8 -p n -f n /dev/ttyACM0
```

`picocom` is passive in this validation. The firmware does not implement a command
shell, UART receive handler, or local echo, so typing produces no response. The test
prints its result once during startup.

To exit `picocom`, press `Ctrl+A`, release the keys, then press `Ctrl+X`.

### 8.4 Capture the validation result

1. Open `picocom`.
2. Flash both cores if they are not already programmed.
3. Press the physical reset button.
4. Capture the complete startup output.

Required success marker:

```text
CCSDSPACK_MCU_TEST:PASS
```

Expected sequence:

```text
CCSDSPack STM32H745 CM7 hardware validation
Running packet generation, parsing, CRC, Manager, Validator, PVN, and Idle tests...
CCSDSPACK_MCU_TEST:PASS
Reset the board to run the validation again.
```

The current test is intentionally concise. It reports a summary and final status rather
than printing every assertion. On failure it prints `CCSDSPACK_MCU_TEST:FAIL:<code>`,
which maps to the failure table in the next section.

The `STM32H745` banner is accepted for the H755 validation and does not invalidate the
result.

LED behavior from the committed harness:

- LED1: test running
- LED2: test passed
- LED3: packet validation or HAL initialization failed

### 8.5 Observed successful hardware run

The NUCLEO-H755ZI-Q validation completed successfully after programming CM4 followed
by CM7. Captured UART evidence:

```text
CCSDSPack STM32H745 CM7 hardware validation
Running packet generation, parsing, CRC, Manager, Validator, PVN, and Idle tests...
CCSDSPACK_MCU_TEST:PASS
Reset the board to run the validation again.
```

## 9. Failure handling

### HAL/startup failure

```text
CCSDSPACK_MCU_TEST:HAL_FAILURE
```

Inspect:

- clock and voltage setup;
- CM4/CM7 startup coordination;
- UART initialization;
- heap/stack and linker layout;
- fault status registers;
- board power and ST-Link configuration.

### Packet test failure

```text
CCSDSPACK_MCU_TEST:FAIL:<code>
```

Failure codes:

| Code | Failure stage |
|---:|---|
| 1 | Set primary header |
| 2 | Register custom secondary header |
| 3 | Set secondary-header bytes |
| 4 | Set Manager template |
| 5 | Manager sequence configuration |
| 6 | Generate application-data packet |
| 7 | Generated wire-vector mismatch |
| 8 | Manager sequence-count advancement |
| 9 | Bounded decode |
| 10 | Consumed-byte count |
| 11 | Decoded fields or CRC mismatch |
| 12 | Validator rejected valid packet |
| 13 | CRC-free primary header |
| 14 | CRC-free application data |
| 15 | CRC-free wire-vector mismatch |
| 16 | CRC-free bounded decode |
| 17 | Non-zero PVN test setup |
| 18 | Non-zero PVN application data |
| 19 | Non-zero PVN was serialized |
| 20 | Invalid Idle Packet setup |
| 21 | Invalid Idle secondary-header setup |
| 22 | Invalid Idle application data |
| 23 | Invalid Idle Packet was serialized |
| 24 | Valid Idle Packet setup |
| 25 | Valid Idle application data |
| 26 | Valid Idle Packet serialization |

Any failure blocks v1.2.0 release until fixed and rerun on the board.

## 10. Record release evidence

Save the following in the release issue or validation record:

- validation date and operator;
- exact `develop` commit SHA;
- MCU TGZ filename and SHA-256;
- `libccsdspack.a` provenance;
- NUCLEO-H755ZI-Q board identification/revision;
- STM32CubeIDE version;
- `arm-none-eabi-g++` version;
- Debug or Release configuration used;
- successful CM7 compile and link;
- ELF flash/RAM/heap/stack usage;
- successful flash and reset;
- complete UART log containing `CCSDSPACK_MCU_TEST:PASS`;
- confirmation that no CPU fault or allocation failure occurred;
- optional photograph/screenshot of UART and LED2.

## 11. Close the v1.2 release gate

After PASS:

1. Update issue `#95` STM32 bare-metal checklist.
2. Update parent issue `#46` with the hardware evidence.
3. Mark the STM32 gate complete on `develop`.
4. Manually merge the validated `develop` state into `main`.
5. Confirm Linux and Windows workflows on the selected `main` commit.
6. Tag `v1.2.0` only after all release gates pass.
7. Publish and verify release artifacts.
8. Close `#95`, then close `#46`.

## Final acceptance statement

The STM32 release gate passes only when the existing CM7 validation suite executes on the NUCLEO-H755ZI-Q and produces:

```text
CCSDSPACK_MCU_TEST:PASS
```

Cross-compilation, archive generation, relocatable linking, or a successful CubeIDE build alone do not satisfy the hardware gate.
