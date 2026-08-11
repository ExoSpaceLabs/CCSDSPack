# STM32H755 hardware-validation integration

The shared CCSDSPack MCU validation core is board-independent and lives in:

```text
CM7/Inc/ccsdspack_mcu_test.h
```

The committed STM32CubeIDE example under `STM32CubeIDE/CM7` is an **STM32H745ZITx / NUCLEO-H745ZI-Q** board harness. Its startup assembly, linker script, device define, and generated CubeIDE metadata must not be reused for an STM32H755 image.

For v2.0.0 release validation, integrate the shared test header into a native **STM32H755ZITx / NUCLEO-H755ZI-Q CM7** project.

## H755 project requirements

The H755 project retains its own:

- `startup_stm32h755xx.s` startup implementation;
- device/system and HAL configuration;
- STM32H755 linker script and memory layout;
- CM4/CM7 boot coordination;
- board clock, power, cache, and MPU setup;
- ST-Link virtual COM UART configuration.

## CCSDSPack build

Build the archive with the same ABI as the H755 application:

```bash
./package.sh \
  -t cmake/toolchains/arm-none-eabi.cmake \
  -p MCU \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
```

The application uses C++17, `CCSDS_MCU`, the installed public headers, `libccsdspack.a`, and matching CPU/FPU/float-ABI flags.

## Execute the shared validation core

After board initialization:

```cpp
#include "ccsdspack_mcu_test.h"

const int result = CCSDSPackMcuTest::run();
```

A successful release run reports:

```text
CCSDSPACK_MCU_TEST:PASS
```

The shared v2 validation core exercises:

- generic Packet construction and exact CRC16 vector generation;
- Manager Packet-template and automatic sequence-count behavior;
- bounded parsing and consumed-byte reporting;
- structured Validator report checks;
- PUS-C telecommand construction, intrinsic direction/Packet Type, serialization, and named PUS validation checks;
- CRC-disabled Packet generation/parsing;
- Packet Version Number rejection;
- Idle Packet constraints.

The generic arm-none-eabi package build also compiles the shared core through `CM7/Src/ccsdspack_mcu_compile_probe.cpp`. Compile/link success proves API and ABI compatibility only; physical H755 execution remains a separate release gate.

## Board-specific responsibilities

The shared test does not configure clocks, voltage scaling, MPU/cache regions, UART, dual-core synchronization, linker layout, heap/stack, fault handlers, or watchdog behavior. Those remain properties of the H755 project and must be validated there.

## Required release evidence

Record:

1. exact NUCLEO-H755ZI-Q board and STM32H755 device revision;
2. arm-none-eabi compiler version;
3. CCSDSPack commit/package SHA;
4. CM7 compile/link success;
5. flash/reset success;
6. UART or debugger result `CCSDSPACK_MCU_TEST:PASS`;
7. final ELF flash/RAM/heap/stack usage;
8. absence of HardFault, MemManage, BusFault, or allocation failure during the run.
