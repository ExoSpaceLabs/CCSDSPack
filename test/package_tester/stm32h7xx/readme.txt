CCSDSPack v1.2.0 STM32H745 CM7 hardware validation
====================================================

Purpose
-------

This project validates the CCSDSPack bare-metal static archive on a real
NUCLEO-H745ZI-Q. It is derived from an STMicroelectronics STM32CubeH7 example,
but the old UART wake-from-STOP demonstration is no longer part of the test.
The validation starts automatically after reset.

The HAL-independent validation core is located at:

  CM7/Inc/ccsdspack_mcu_test.h

The generic arm-none-eabi package build compiles the same core through:

  CM7/Src/ccsdspack_mcu_compile_probe.cpp

That compile-only check catches stale API calls and MCU preprocessor errors.
Only execution on the STM32 validates startup, final linking, C++ runtime,
heap behavior, UART reporting, and operation on silicon.

Target
------

- Board: NUCLEO-H745ZI-Q
- Device: STM32H745ZITx
- Core under test: Cortex-M7
- CPU flags: cortex-m7, Thumb, FPv5-D16, hard-float ABI
- Language: C++17
- Exceptions: disabled
- RTTI: disabled

Build the MCU package
---------------------

From the CCSDSPack repository root:

  ./package.sh \
    -t cmake/toolchains/arm-none-eabi.cmake \
    -p MCU \
    -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"

The archive contains the installed public headers and `libccsdspack.a`.
The application and library must use the same CPU, FPU, float-ABI, exception,
RTTI, and `CCSDS_MCU` settings.

STM32CubeIDE configuration
--------------------------

Extract or copy the package into a stable directory. A convenient project-local
layout is:

  test/package_tester/stm32h7xx/
    Middlewares/Third_Party/CCSDSPack/
      include/CCSDSPack.h
      lib/libccsdspack.a

For both Debug and Release CM7 configurations, verify these settings in
STM32CubeIDE. Do not rely on paths generated on another workstation.

C++ compiler:

- Preprocessor symbol: `CCSDS_MCU`
- Include path from the CM7 build directory:

    ../../../Middlewares/Third_Party/CCSDSPack/include

- Language standard: GNU C++17 or C++17
- Other flags:

    -fno-exceptions -fno-rtti -fno-use-cxa-atexit

C++ linker:

- Library search path from the CM7 build directory:

    ../../../Middlewares/Third_Party/CCSDSPack/lib

- Library name:

    ccsdspack

This corresponds to `libccsdspack.a`. The obsolete library name
`ccsdspack_mcu` is not produced by the current CMake build.

Important: older committed/generated STM32CubeIDE metadata may contain absolute
`/home/dev/...` paths. Replace them with the project-local paths above before
building. Generated Debug/Release makefiles and compile_commands files should
not be treated as portable configuration.

Memory model
------------

CCSDSPack's API is exception-free, but it is not allocation-free. The library
and this validation use `std::vector`, `std::string`, `std::shared_ptr`, and the
secondary-header factory. A working newlib heap and sufficient RAM are required.

The supplied linker script reserves a minimum stack area and permits `_sbrk()`
to grow the heap from `_end` toward the reserved stack boundary. Review heap and
stack usage for the final mission application; a successful smoke test is not a
worst-case memory proof.

Build, flash, and observe
-------------------------

1. Import/open the dual-core STM32CubeIDE project.
2. Build the CM7 image with the configuration above.
3. Flash the board using ST-Link/STM32CubeProgrammer.
4. Connect to the ST-Link virtual COM port:

     picocom -b 115200 -d 8 -p n -f n /dev/ttyACM0

5. Reset the board.

Expected successful output:

  CCSDSPack STM32H745 CM7 hardware validation
  Running packet generation, parsing, CRC, Manager, Validator, PVN, and Idle tests...
  CCSDSPACK_MCU_TEST:PASS
  Reset the board to run the validation again.

LED behavior:

- LED1: test is running
- LED2: validation passed
- LED3: validation or HAL initialization failed

A packet-test failure is printed as:

  CCSDSPACK_MCU_TEST:FAIL:<code>

A board/HAL initialization failure is printed as:

  CCSDSPACK_MCU_TEST:HAL_FAILURE

Failure codes
-------------

  1  set primary header
  2  register custom secondary header
  3  set custom secondary header bytes
  4  set Manager template
  5  Manager sequence configuration
  6  generate application-data packet
  7  generated wire vector mismatch
  8  Manager sequence-count advancement
  9  bounded decode
  10 consumed-byte count
  11 decoded logical fields or CRC
  12 Validator rejected valid packet
  13 CRC-free primary header
  14 CRC-free application data
  15 CRC-free wire vector
  16 CRC-free bounded decode
  17 non-zero PVN test setup
  18 non-zero PVN application data
  19 non-zero PVN was serialized
  20 invalid Idle Packet setup
  21 invalid Idle secondary-header setup
  22 invalid Idle application data
  23 invalid Idle Packet was serialized
  24 valid Idle Packet setup
  25 valid Idle application data
  26 valid Idle Packet serialization

What the test covers
--------------------

- Cortex-M7 consumer compilation with `CCSDS_MCU`
- custom variable-length secondary-header registration
- Manager template and automatic sequence-count advancement
- exact independent CRC16 packet vector
- bounded parsing and consumed-byte reporting
- decoded primary-header, secondary-header, application-data, and CRC fields
- Validator template/coherence checks
- CRC-disabled packet generation and parsing
- rejection of non-zero Packet Version Number serialization
- Idle Packet restrictions and valid Idle Packet generation
- C++ container/shared-ownership allocation on the configured target runtime

The test does not prove timing bounds, fragmentation behavior over long mission
operation, worst-case memory use, interrupt safety, thread safety, radiation
tolerance, or suitability of the project's linker layout for another STM32.
