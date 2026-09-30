# STM32H755 hardware validation with DAS + OpenOCD

This is the release hardware harness for **NUCLEO-H755ZI-Q / Cortex-M7**.

It deliberately does **not** use STM32CubeIDE, STM32 HAL, generated BSP projects,
EWARM, MDK, or a companion CM4 image.

Platform responsibilities are delegated to
[device-abstraction-stack](https://github.com/Inczert/device-abstraction-stack)
and only CMSIS headers are consumed from STM32CubeH7.

## Architecture

```text
CCSDSPack acceptance test
        |
        v
C++17 test firmware
        |
        +---- CCSDSPack MCU package (artifact under test)
        |
        +---- DAS
               |
               +---- startup / vector table
               +---- clock control
               +---- board LED resources
               +---- ST-LINK VCP UART
               +---- STM32H755 CMSIS backend
        |
        v
OpenOCD + ST-LINK
        |
        v
NUCLEO-H755ZI-Q CM7
```

DAS UART resource:

```text
DAS_BOARD_UART_STLINK_VCP
USART3 / AF7
PD8  TX
PD9  RX
115200 8N1
```

## Pinned platform baseline

The harness pins:

```text
device-abstraction-stack
bf0af2a0c53d361ed8bb271b4160dd3967b28af8

STM32CubeH7
f5c0b7a2b1f6eb26fde150f72edb2d7deb647066

cmsis-device-h7
e8d40ae6e2fa06afe5b46d24756f141b363342a7
```

STM32CubeH7 is used for CMSIS headers only. No HAL/LL source is compiled.

## Requirements

```text
cmake
git
tar
sha256sum
gcc-arm-none-eabi / arm-none-eabi-g++
openocd
stty
```

The board's ST-LINK USB connection must expose both SWD and the virtual COM port.

## Build the exact MCU package

From the repository root:

```bash
bash test/package_tester/stm32h755_das/build.sh \
  --package packages/ccsdspack-v2.1.0-Generic-arm.tar.gz \
  --source-sha "$(git rev-parse HEAD)"
```

When no STM32CubeH7 path is supplied, the script fetches only the pinned CMSIS
dependencies under `build/stm32h755-das-deps/`.

The result is:

```text
build/stm32h755-das-validation/firmware/
  ccsdspack_stm32h755_validation.elf
  ccsdspack_stm32h755_validation.map
  size-and-identity.txt
```

The firmware links the **extracted package archive**, not the CCSDSPack source-tree
library. The UART report embeds the supplied source SHA, package SHA-256,
`libccsdspack.a` SHA-256, DAS SHA, compiler identity, target clock and UART baud.

## Flash and run

Usually the ST-LINK VCP is auto-detected:

```bash
bash test/package_tester/stm32h755_das/run.sh
```

Specify it explicitly when several serial devices are connected:

```bash
bash test/package_tester/stm32h755_das/run.sh \
  --uart /dev/serial/by-id/<ST-LINK-VCP>
```

The runner:

1. configures the UART as 115200 8N1;
2. starts UART capture;
3. connects under reset through OpenOCD;
4. flashes and verifies the ELF;
5. resets CM7 into the test;
6. waits for an explicit PASS/FAIL/FAULT marker;
7. retains the complete UART log.

## UART evidence

A successful run is intentionally verbose:

```text
CCSDSPACK_HARDWARE_TEST:BEGIN
CCSDSPACK_VERSION:2.1.0
CCSDSPACK_SOURCE_SHA:<sha>
CCSDSPACK_PACKAGE_SHA256:<sha256>
CCSDSPACK_LIBRARY_SHA256:<sha256>
CCSDSPACK_DAS_SHA:<sha>
CCSDSPACK_BOARD:NUCLEO-H755ZI-Q
CCSDSPACK_CORE:Cortex-M7
CCSDSPACK_TRANSPORT:DAS ST-LINK VCP / USART3 PD8-PD9
CCSDSPACK_UART_FORMAT:115200 8N1
CCSDSPACK_COMPILER:<arm-none-eabi compiler>
CCSDSPACK_CORE_HZ:400000000
CCSDSPACK_UART_EFFECTIVE_BAUD:<measured>
CCSDSPACK_HEAP_CAPACITY_BYTES:<bytes>
CCSDSPACK_TEST:generic_packet:BEGIN
CCSDSPACK_TEST:generic_packet:PASS
...
CCSDSPACK_TEST:all:PASS
CCSDSPACK_HEAP_USED_BYTES:<bytes>
CCSDSPACK_HEAP_PEAK_BYTES:<bytes>
CCSDSPACK_RESULT_CODE:0
CCSDSPACK_RESULT_NAME:PASS
CCSDSPACK_HARDWARE_TEST:PASS
```

Failure reports include the first acceptance code/name. Cortex-M faults emit a
`CCSDSPACK_HARDWARE_TEST:FAULT:<name>` marker when UART is already available.

## LEDs

DAS semantic board LEDs are used only as secondary visual evidence:

- yellow: validation running;
- green: PASS;
- red: FAIL or fault.

UART is the authoritative machine-readable release evidence.

## Release rule

The hardware run must use the MCU package produced from the exact candidate commit.
If code, build logic, dependency pins, or this harness changes after qualification,
the candidate SHA changes and hardware qualification must be repeated.
