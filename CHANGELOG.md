<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Changelog

All notable changes to CCSDSPack are documented in this file. Upgrade-specific v1.2-to-v2.0 source/configuration mapping is maintained in `docs/MIGRATION_V1_TO_V2.md`.

## [Unreleased] - v2.0.0

### Packet and PUS support

- CCSDS 133.0-B-2 EC2 Space Packet PDU construction, checked serialization, bounded transactional parsing, full APID handling, Idle Packet validation, segmentation, and modulo-16384 sequence handling.
- Standards-oriented PUS-A and PUS-C TC/TM secondary-header codecs under `ccsds::pus::rev_a` and `ccsds::pus::rev_c`.
- Concrete PUS types with intrinsic revision/direction and direction-specific optional tailoring.
- Generic Packet-level `PacketErrorControlMode::{CRC16,None}` and configurable CRC-16/CCITT-FALSE parameters.
- Numeric basic CCSDS CUC time with explicit epoch metadata, P-field mode, coarse width, and fine width.

### Validation and parsing

- Fixed-capacity `ccsds::ValidationReport` with named `ccsds::ValidationCode` checks.
- Packet-template validation for Packet Identification, segmentation class, packet error control, and secondary-header contract.
- `ccsds::buffer::declaredPacketSize()` and pointer-plus-size Packet/Manager adapters.
- Typed PUS vector/raw parsing and runtime canonical selectors.
- Allocation-free symbolic `errorCodeName()` / `validationCodeName()` labels.

### Integration

- Complete Packet templates as the `ccsds::Manager` generation/receive contract.
- Host-side encoder, decoder, validator, and typed configuration support.
- Installed CMake package and standalone `find_package()` examples.
- C++17 `CCSDS_MCU` static-library build supporting `-fno-exceptions -fno-rtti`.
- Linux, Windows, Doxygen, CLI, installed-consumer/example, and package/cross-build gates.

### Validation status

- 125 native regression/conformance tests on the current integration candidate.
- Local ASan and UBSan runs completed.
- Final sanitizer/fuzz CI, PUS-C vector traceability, negative-vector approval, arm64 execution, STM32 execution, and release-publication verification remain v2.0.0 release gates.

## [1.2.0] - 2026-08-02

### Added

- CCSDS 133.0-B-2 Issue 2 plus Editorial Change 2 Space Packet PDU profile and compliance traceability.
- `PacketErrorControlMode::None`, bounded parsing with consumed-byte reporting, exact `getSerializedSize()`, independent golden vectors, installed-package consumer validation, and Raspberry Pi 5 / STM32H755 release evidence.

### Changed

- Correct Packet Data Length semantics and packet-boundary handling.
- CRC16 coverage/validation, sequence-count handling, complete Packet Identification binding, APID/Idle behavior, and non-mutating inspection were hardened for the documented v1.2 packet profile.

### Validation

- Ubuntu 22.04/24.04/latest, Windows, CLI integration, package/cross-build, independent vectors, Raspberry Pi 5 arm64, and NUCLEO-H755ZI-Q validation completed for v1.2.0.

## Earlier releases

Earlier release details remain available in the repository history and GitHub Releases.

[1.2.0]: https://github.com/ExoSpaceLabs/CCSDSPack/compare/v1.1.1...v1.2.0
