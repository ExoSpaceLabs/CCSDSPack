# STM32H755 hardware validation

CCSDSPack validates the physical Cortex-M7 target with a small standalone
CMake/OpenOCD harness. The old STM32CubeIDE/HAL/BSP example project is not part
of the validation path.

The release test reuses the same board-independent acceptance body as native
arm64:

```text
test/package_tester/hardware/ccsdspack_hardware_test.h
```

## Architecture

```text
CCSDSPack hardware acceptance
        |
        +-- CCSDSPack v2.1 C++17 API
        |      |
        |      +-- authoritative C11 protocol core
        |
        +-- DAS board support
               |
               +-- Cortex-M startup / vector table
               +-- STM32H755 clock + device support
               +-- NUCLEO-H755ZI-Q board resources
               +-- linker script
               +-- ST-LINK VCP UART

host
  |
  +-- arm-none-eabi + CMake
  +-- OpenOCD / ST-LINK
  +-- serial capture, 115200 8N1
```

There is no dependency on STM32CubeIDE, generated Eclipse projects, STM32 HAL,
the Nucleo BSP, vendor IDE makefiles, or a CM4 companion image.

DAS uses the NUCLEO-H755ZI-Q ST-LINK virtual COM route directly:

```text
DAS_BOARD_UART_STLINK_VCP
PD8 TX / PD9 RX
USART3 / AF7
115200 8N1
```

The only STM32CubeH7 material fetched by the runner is the pinned CMSIS core
and STM32H755 device-header subset required by DAS.

## Pinned dependencies

The validation runner pins its target substrate rather than building against
whatever happens to be installed on a workstation:

```text
DAS
b10fa1e8ceb021c406d0c15c7020c0114fe0469f

STM32CubeH7 CMSIS source
f5c0b7a2b1f6eb26fde150f72edb2d7deb647066
```

Those are the same DAS/STM32H7 baselines used by the SpWKit STM32H755 DAS
integration.

## Build-only validation

The hardware harness is itself cross-built in CI from the generated MCU
package. This proves that the package links as a complete H755 firmware image,
not merely as a relocatable probe.

```bash
bash test/package_tester/stm32h7xx/run_h755_validation.sh \
  --package /path/to/ccsdspack-v2.1.0-Generic-arm.tar.gz \
  --source-sha <candidate-sha> \
  --build-only
```

The build produces:

```text
ccsdspack_h755_validation.elf
ccsdspack_h755_validation.map
logs/metadata.log
logs/size.log
logs/elf.sha256
```

The ELF is rejected if it retains an STM32 HAL or BSP dependency.

## Physical validation

Requirements:

- NUCLEO-H755ZI-Q connected through ST-LINK;
- ARM GNU bare-metal toolchain;
- CMake and Git;
- OpenOCD with ST-LINK support;
- access to the ST-LINK virtual COM port;
- the exact CCSDSPack MCU package generated for the candidate commit.

Run:

```bash
bash test/package_tester/stm32h7xx/run_h755_validation.sh \
  --package /path/to/ccsdspack-v2.1.0-Generic-arm.tar.gz \
  --source-sha <candidate-sha>
```

The runner normally auto-detects the ST-LINK virtual COM device. It can be
selected explicitly when several ACM devices are present:

```bash
bash test/package_tester/stm32h7xx/run_h755_validation.sh \
  --package /path/to/ccsdspack-v2.1.0-Generic-arm.tar.gz \
  --source-sha <candidate-sha> \
  --uart /dev/ttyACM0
```

The runner:

1. hashes the supplied CCSDSPack package;
2. checks out the exact DAS revision;
3. sparse-fetches only the required pinned STM32 CMSIS headers;
4. builds and installs DAS for Cortex-M7;
5. links the CCSDSPack validation ELF from the supplied MCU package;
6. records ELF size and SHA-256;
7. opens the ST-LINK VCP at 115200 8N1;
8. programs and verifies the ELF using OpenOCD;
9. releases the target;
10. captures the full UART transcript;
11. fails on FAIL/FAULT, timeout, or absence of the final PASS marker.

## UART evidence

UART is the authoritative human-readable hardware transcript. A successful run
contains metadata before the protocol tests:

```text
=== CCSDSPack STM32H755 hardware validation ===
BOARD:NUCLEO-H755ZI-Q
CORE:Cortex-M7
TRANSPORT:DAS UART / ST-LINK VCP
UART_FORMAT:115200 8N1
CCSDSPACK_SOURCE_SHA:<candidate-sha>
CCSDSPACK_PACKAGE_SHA256:<package-sha256>
DAS_SHA:b10fa1e8ceb021c406d0c15c7020c0114fe0469f
COMPILER:<arm-none-eabi compiler version>
CPP_STANDARD:201703
CORE_HZ:<measured DAS core clock>
UART_EFFECTIVE_BAUD:<effective DAS UART baud>
ACCEPTANCE:Packet, Manager, CRC16, raw buffers, Validator, PUS-C, PVN, Idle
CCSDSPACK_HARDWARE_TEST:BEGIN
```

Each major acceptance section then reports progress:

```text
TEST:BEGIN:generic-packet-manager
TEST:PASS:generic-packet-manager
TEST:BEGIN:raw-buffer-parse
TEST:PASS:raw-buffer-parse
TEST:BEGIN:structured-validator
TEST:PASS:structured-validator
TEST:BEGIN:raw-manager-reassembly
TEST:PASS:raw-manager-reassembly
TEST:BEGIN:pus-c-tc
TEST:PASS:pus-c-tc
TEST:BEGIN:pec-none
TEST:PASS:pec-none
TEST:BEGIN:packet-version-rejection
TEST:PASS:packet-version-rejection
TEST:BEGIN:idle-packet-policy
TEST:PASS:idle-packet-policy
TEST:PASS:all
RESULT_CODE:0
RESULT_NAME:Pass
CCSDSPACK_HARDWARE_TEST:PASS
CCSDSPACK_HARDWARE_TEST:END
```

Failures include both numeric and symbolic identity:

```text
RESULT_CODE:<n>
RESULT_NAME:<ResultCode name>
CCSDSPACK_HARDWARE_TEST:FAIL:<n>:<ResultCode name>
```

Target faults are also emitted over UART before the core halts:

```text
FAULT:HardFault
CCSDSPACK_HARDWARE_TEST:FAULT
```

The same applies to MemManage, BusFault and UsageFault.

## Shared protocol coverage

The board-independent acceptance body exercises:

- generic Packet construction and exact CRC16 vector generation;
- Manager Packet-template and automatic sequence-count behavior;
- packet-level PEC with CRC16 and `None`;
- pointer-native declared-size and bounded parsing;
- truncated raw-buffer rejection;
- structured Validator report checks;
- raw Manager stream loading and application-data reconstruction;
- PUS-C telecommand construction, serialization, typed parsing and validation;
- Packet Version Number rejection;
- Idle Packet constraints.

The generic MCU package compile probe remains under
`CM7/Src/ccsdspack_mcu_compile_probe.cpp` for package/ABI CI. Physical H755
execution uses `CM7/Src/main.cpp` and is a separate release gate.

## Release evidence

Retain from every release-candidate hardware run:

1. exact CCSDSPack candidate SHA;
2. exact MCU package filename and SHA-256;
3. pinned DAS SHA;
4. compiler identity printed by the target;
5. final ELF SHA-256;
6. final ELF `text/data/bss` size;
7. complete UART transcript;
8. OpenOCD program/verify log;
9. final `CCSDSPACK_HARDWARE_TEST:PASS` marker;
10. absence of FAIL/FAULT markers.

A source, package, DAS pin, linker/startup, board-support, or validation-harness
change creates a new hardware candidate and requires fresh physical evidence.
