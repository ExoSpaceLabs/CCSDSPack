CCSDSPack v2 Cortex-M7 validation harness
=========================================

Purpose
-------

This directory contains the shared CCSDSPack C++17 MCU validation core and a
committed NUCLEO-H745ZI-Q/STM32H745ZITx board harness. The same shared core is
used by the generic arm-none-eabi compile/link probe and is intended to be
integrated into the NUCLEO-H755ZI-Q project used for final v2.0.0 hardware
release evidence.

Shared validation core:

  CM7/Inc/ccsdspack_mcu_test.h

Compile/link probe:

  CM7/Src/ccsdspack_mcu_compile_probe.cpp

The H745 CubeIDE project proves only that board when executed. H755 release
validation must use a native H755 startup, linker script, device configuration,
and board initialization as described in H755_INTEGRATION.md.

Library build
-------------

  ./package.sh \
    -t cmake/toolchains/arm-none-eabi.cmake \
    -p MCU \
    -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"

The application and library must use C++17, CCSDS_MCU, matching Cortex-M7/FPU
ABI flags, -fno-exceptions, and -fno-rtti. The archive is libccsdspack.a.

Runtime and memory model
------------------------

CCSDSPack uses an exception-free Result/Error API for normal failures, but the
complete library is not allocation-free. Packet/Manager/PUS paths use standard
C++ containers/shared ownership. A working heap and sufficient RAM are required.
ValidationReport itself uses fixed std::array storage and performs no dynamic
allocation.

Shared test coverage
--------------------

The shared v2 core covers:

- generic Packet construction and exact independent CRC16 bytes;
- custom variable-length secondary-header registration;
- Manager Packet-template and automatic sequence-count behavior;
- bounded parsing and consumed-byte reporting;
- decoded primary-header, secondary-header, application-data, and CRC fields;
- structured Validator PacketDataLength/CRC/identifier/template checks;
- PUS-C telecommand construction and intrinsic Telecommand Packet Type;
- PUS revision/direction/tailoring/acknowledgement/source-ID validation checks;
- CRC-disabled Packet generation and parsing;
- non-zero Packet Version Number rejection;
- Idle Packet rejection/acceptance cases;
- standard-container/shared-ownership runtime operation on the target.

Result codes
------------

  0  pass
  1  set primary header
  2  register custom secondary header
  3  set custom secondary header
  4  set Manager template
  5  Manager sequence configuration
  6  set application data
  7  generated wire vector
  8  Manager sequence advancement
  9  bounded decode
  10 bounded consumed-byte count
  11 decoded fields
  12 Validator rejected packet
  13 CRC-free header
  14 CRC-free data
  15 CRC-free wire vector
  16 CRC-free decode
  17 invalid-version header setup
  18 invalid-version data setup
  19 invalid version serialized
  20 invalid Idle header setup
  21 invalid Idle secondary header setup
  22 invalid Idle data setup
  23 invalid Idle packet serialized
  24 valid Idle header setup
  25 valid Idle data setup
  26 valid Idle serialization
  27 structured Validator report missing
  28 PUS header setup
  29 PUS direction inference
  30 PUS application data
  31 PUS serialization
  32 PUS Validator rejection
  33 PUS Validator named checks missing

Physical execution
------------------

A successful board run reports:

  CCSDSPACK_MCU_TEST:PASS

Compile/link success is useful CI evidence but does not prove startup, final
link/runtime memory behavior, UART reporting, or operation on silicon. Final
v2.0.0 release acceptance records a fresh NUCLEO-H755ZI-Q execution separately.

The test is not a proof of timing bounds, long-duration fragmentation, worst-case
memory use, interrupt/thread safety, radiation tolerance, or suitability of any
committed board linker layout for a different target.
