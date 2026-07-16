# STM32H755 hardware-validation integration

The committed STM32CubeIDE example under `STM32CubeIDE/CM7` targets the
**STM32H745ZITx / NUCLEO-H745ZI-Q**. Do not reuse its startup assembly, linker
script, device define, or generated CubeIDE metadata for an STM32H755 image.

The CCSDSPack validation logic itself is board-independent and lives in:

```text
CM7/Inc/ccsdspack_mcu_test.h
```

Use that header from a native **STM32H755ZITx / NUCLEO-H755ZI-Q CM7** project.

## Required H755 project setup

Create or use a working STM32H755 CM7 project generated for the exact board and
retain its own:

- `startup_stm32h755xx.s` startup implementation;
- STM32H755 system file and HAL configuration;
- STM32H755 linker script and memory layout;
- CM4/CM7 boot coordination;
- board-specific clock and power setup;
- ST-Link virtual COM UART configuration.

Do not copy `STM32H745ZITX_FLASH.ld` into the H755 project.

## CCSDSPack build compatibility

Build the archive using the same ABI as the H755 CM7 application:

```bash
./package.sh \
  -t cmake/toolchains/arm-none-eabi.cmake \
  -p MCU \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
```

The application configuration must use:

- C++17;
- `CCSDS_MCU`;
- `-fno-exceptions`;
- `-fno-rtti`;
- the same `-mcpu`, `-mfpu`, `-mfloat-abi`, and Thumb settings as the archive;
- the installed package headers;
- `libccsdspack.a`, linked as `ccsdspack`.

## Calling the validation core

After the H755 project has completed HAL, clock, memory, UART, and LED
initialization, run:

```cpp
#include "ccsdspack_mcu_test.h"

const int result = CCSDSPackMcuTest::run();
```

Report the result deterministically over UART, debugger memory, or LEDs. The
release gate expects the equivalent of:

```text
CCSDSPACK_MCU_TEST:PASS
```

A non-zero value is one of the documented failure codes in `readme.txt`.

## What remains board-specific

The shared validation core does not configure:

- clocks or voltage scaling;
- MPU/cache regions;
- UART pins and peripheral clocks;
- CM4/CM7 boot synchronization;
- the linker script, heap, or stack;
- fault handlers or watchdog behavior.

Those must remain native to the H755 project. This separation is intentional:
the same packet test is compiled in CI and executed on hardware, while the board
project remains responsible for proving its own startup, runtime, and memory
configuration.

## H755 release evidence

Record at least:

1. exact NUCLEO-H755ZI-Q board and STM32H755 device revision;
2. arm-none-eabi compiler version;
3. archive/package commit SHA;
4. CM7 compile and link success;
5. flash and reset success;
6. UART or debugger result `CCSDSPACK_MCU_TEST:PASS`;
7. final ELF flash/RAM/heap/stack usage;
8. absence of HardFault, MemManage, BusFault, or allocation failure during the test.
