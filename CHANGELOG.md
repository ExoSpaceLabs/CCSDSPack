<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Changelog

All notable changes to CCSDSPack are documented here. Upgrade-specific v1.2-to-v2
source/configuration mapping is maintained in `docs/MIGRATION_V1_TO_V2.md`.

## [Unreleased] - v2.1.0 candidate

### Architecture

- Added an authoritative C11 protocol core for primary-header handling, packet views,
  packet finalization/encoding, CRC16, CUC, PUS-A/PUS-C codecs, validation,
  segmentation/reassembly, and stream primitives.
- Preserved the established C++17 Packet/Manager ownership and convenience API.
- Added installed `ccsdspack::c` CMake target and a C-only build/consumer path that
  does not require a C++ compiler or runtime.
- Removed avoidable bridge copies and aggregate temporaries from key C++ parsing and
  serialization paths.

### Performance and footprint

- Matched v2.0-main vs v2.1 Release benchmarks show owning raw parse improvements of
  1.6x to 4.8x without CRC and 1.02x to 1.19x with CRC across measured packet sizes.
- Owning raw parse allocations were reduced from 9/11 to 3 per operation.
- Public vector-returning packet serialization was reduced to one allocation in the
  measured cached/refinalized cases.
- The low-level C packet view remains allocation-free and performs no transport-adapter
  bridge copy.
- Matched Cortex-M7 retained text increased by 3,928 bytes (+10.8%) in the documented
  `--gc-sections` comparison.
- Broader matched PUS/CUC/Validator/Manager/stream benchmarks are tracked in issue #163
  before broad whole-core speed claims are made.

### Standards and validation

- Preserved the v2 compliance scope for CCSDS 133.0-B-2 Issue 2 + Editorial Change 2,
  ECSS-E-70-41A PUS-A, ECSS-E-ST-70-41C PUS-C, and the supported CCSDS 301.0-B-4 CUC subset.
- Preserved independent wire-vector evidence, including the complete PUS-C TC
  acknowledgement matrix `0x0..0xF`.
- Expanded the native regression/conformance suite to **134/134 passing tests**.
- Preserved structured negative validation evidence for all 26 public validation codes.
- Kept dedicated ASan, UBSan, and bounded libFuzzer robustness gates.

### Hardware and packaging

- Added standalone DAS/OpenOCD NUCLEO-H755ZI-Q validation with no STM32CubeIDE/HAL/BSP dependency.
- Qualified the hard-float Cortex-M7 startup path and recorded physical
  `CCSDSPACK_HARDWARE_TEST:PASS`.
- Added one-command Raspberry Pi 5/native arm64 package validation and recorded
  `CCSDSPACK_AARCH64_TEST:PASS`.
- Hardened package/version/architecture identity checks and release evidence capture.

## [2.0.0] - 2026-08-31

### Highlights

- Established the v2 C++17 Packet/Manager API and standards-oriented PUS-A/PUS-C model.
- Added bounded transactional parsing, structured validation, Packet-level PEC policy,
  numeric CUC support, installed-package consumers, robustness gates, and hardware evidence.
- Published native x86_64/arm64 packages, Cortex-M MCU package, GitHub Release assets,
  and GHCR images from the approved tag.

## [1.2.0] - 2026-08-02

### Added

- CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 Space Packet PDU profile and compliance traceability.
- `PacketErrorControlMode::None`, bounded parsing with consumed-byte reporting,
  exact `getSerializedSize()`, independent golden vectors, installed-package consumer
  validation, and Raspberry Pi 5 / STM32H755 release evidence.

### Changed

- Correct Packet Data Length semantics and packet-boundary handling.
- CRC16 coverage/validation, sequence-count handling, complete Packet Identification
  binding, APID/Idle behavior, and non-mutating inspection were hardened.

## Earlier releases

Earlier release details remain available in repository history and GitHub Releases.
