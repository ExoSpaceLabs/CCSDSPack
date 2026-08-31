# STM32H755 hardware-validation integration

CCSDSPack uses one board-independent release acceptance core for both native arm64 and physical Cortex-M7 validation:

```text
test/package_tester/hardware/ccsdspack_hardware_test.h
```

The STM32 wrapper remains available at:

```text
test/package_tester/stm32h7xx/CM7/Inc/ccsdspack_mcu_test.h
```

The committed STM32CubeIDE example under `STM32CubeIDE/CM7` uses the STM32CubeH7 **NUCLEO-H745ZI-Q** project configuration. ST documents all projects under `Projects/NUCLEO-H745ZI-Q` as fully compatible with the **NUCLEO-H755ZI-Q** board, so the H745 startup/linker/device names used by that project are valid for the non-cryptographic CCSDSPack H755 validation path.

Official ST compatibility note:

`https://github.com/STMicroelectronics/STM32CubeH7/blob/master/Projects/NUCLEO-H755ZI-Q/readme.txt`

The H755-specific cryptographic capability called out by ST is outside CCSDSPack's scope.

For v2.0.0 release validation, execute the shared acceptance core on a physical **NUCLEO-H755ZI-Q / Cortex-M7** using the ST-supported H745/H755-compatible project configuration. The same `CCSDSPackHardwareTest::run()` implementation is executed natively by `aarch64_validate.sh`, so the two real-target runs exercise the same protocol/API acceptance logic.

## Board-project requirements

The STM32 project remains responsible for:

- startup implementation and linker/memory layout supplied by the ST-compatible board project;
- device/system and HAL configuration;
- CM4/CM7 boot coordination;
- board clock, power, cache, and MPU setup;
- ST-Link virtual COM UART configuration.

## CCSDSPack build

Build the archive with the same ABI as the Cortex-M7 application:

```bash
./package.sh \
  -t cmake/toolchains/arm-none-eabi.cmake \
  -p MCU \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
```

The application uses C++17, `CCSDS_MCU`, the installed public headers, `libccsdspack.a`, and matching CPU/FPU/float-ABI flags.

## Execute the shared validation core

Add `test/package_tester/hardware` to the application include path and call the same acceptance function used on arm64:

```cpp
#include "ccsdspack_hardware_test.h"

const int result = CCSDSPackHardwareTest::run();
```

A successful release run should report through UART/debugger:

```text
CCSDSPACK_HARDWARE_TEST:PASS
```

The compatibility wrapper can also be used from the in-repository STM32 example:

```cpp
#include "ccsdspack_mcu_test.h"
const int result = CCSDSPackMcuTest::run();
```

## Shared acceptance coverage

The exact same acceptance core used on native arm64 and STM32 CM7 exercises:

- generic Packet construction and exact CRC16 vector generation;
- Manager Packet-template and automatic sequence-count behavior;
- packet-level PEC with CRC16 and `None`;
- structured Validator report checks;
- PUS-C telecommand construction, intrinsic direction/Packet Type, serialization, typed parsing, and named PUS validation checks;
- Packet Version Number rejection and Idle Packet constraints;
- **raw application-data ingestion through `Manager::setApplicationData(const uint8_t*, size_t)`**;
- **six-byte framing through `ccsds::buffer::declaredPacketSize()`**;
- **pointer-plus-size bounded generic Packet parsing with exact consumed-byte checks**;
- **truncated raw-buffer rejection**;
- **typed PUS-C raw-buffer parsing**;
- **raw Manager stream loading and application-data reconstruction**.

This means the physical H755 run and native arm64 run both explicitly validate the transport-facing raw-buffer APIs rather than relying only on hosted unit tests or compile/link probes.

The generic arm-none-eabi package build compiles the same core through `CM7/Src/ccsdspack_mcu_compile_probe.cpp`. Compile/link success proves API and ABI compatibility only; physical NUCLEO-H755ZI-Q execution remains a separate release gate.

## Board-specific responsibilities

The shared test does not configure clocks, voltage scaling, MPU/cache regions, UART, dual-core synchronization, linker layout, heap/stack, fault handlers, or watchdog behavior. Those remain properties of the STM32 board project and must be validated there.

## Required release evidence

Record:

1. physical NUCLEO-H755ZI-Q board identity and, when available, MCU revision information reported by the programmer/debugger;
2. arm-none-eabi compiler version;
3. CCSDSPack commit/package SHA;
4. CM7 compile/link success;
5. flash/reset success;
6. UART or debugger result `CCSDSPACK_HARDWARE_TEST:PASS`;
7. final ELF flash/RAM usage and relevant heap/stack configuration;
8. absence of HardFault, MemManage, BusFault, or allocation failure during the run.

For arm64, retain the complete output from `test/package_tester/aarch64_validate.sh`; its final successful run includes `CCSDSPACK_HARDWARE_TEST:PASS` followed by `CCSDSPACK_AARCH64_TEST:PASS`.
