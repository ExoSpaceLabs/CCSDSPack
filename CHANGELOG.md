<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Changelog

All notable changes to CCSDSPack are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses semantic versioning for its public package versions.

## [Unreleased] - v2.0.0

### Added

- Standards-oriented PUS-A and PUS-C TC/TM secondary-header codecs under `ccsds::pus::rev_a` and `ccsds::pus::rev_c`.
- Explicit `ccsds::MissionProfile` selection and validation for generic CCSDS and PUS revision/direction profiles.
- Numeric basic CCSDS CUC time with explicit epoch, P-field, coarse-width, and fine-width policy.
- Fixed-capacity `ccsds::ValidationReport` and named `ccsds::ValidationCode` checks for generic CCSDS, templates, mission profiles, and supported PUS fields.
- Standalone installed-package examples and complete generic/PUS configuration profiles.

### Changed

- Public C++ namespace changes from `CCSDS` to `ccsds`.
- Packet finalization and serialization APIs return checked Result types rather than hiding failure behind empty buffers.
- Public secondary-header API uses `SecondaryHeader` terminology instead of the previous `DataFieldHeader` naming.
- `ccsds::Validator::validate()` returns a structured named report instead of a positional six-element boolean report.
- `ccsds_validator` delegates protocol/profile checks to the library Validator.
- PUS packet configuration requires explicit revision, TC/TM direction, canonical selector, identifier widths, packet error control, and applicable CUC policy.
- The bare-metal package path forwards `-m` flags through `CCSDSPACK_MCU_FLAGS` to the library and consumer probe.

### Removed

- Legacy project-specific `PusA`, `PusB`, and `PusC` classes.
- `PusServices` and automatic registration of legacy pseudo-PUS formats.
- Automatic UML generation as a v2.0.0 CI/release gate. The workflow remains available manually through `workflow_dispatch`.

### Compatibility

v2.0.0 is a breaking source/API and PUS-layout release. The corrected generic Space Packet wire behavior delivered in v1.2.0 is retained and is not reclassified as a new v2 wire break.

The project remains C++17. The protocol-facing Validator, MissionProfile, PUS, CUC, Packet, Manager, and Result APIs remain available in the `CCSDS_MCU` static-library build and are compatible with builds that disable exceptions and RTTI.

### Validation status

Current development evidence includes 108 passing hosted native tests, Linux/Windows hosted builds, Doxygen, representative generic/PUS CLI integration, local ASan/UBSan runs, and an ARM compile/link consumer probe that exercises the structured Validator and a PUS-C telecommand path. Final v2 release gates remain tracked by the v2 milestone.

## [1.2.0] - 2026-08-02

### Added

- A documented **CCSDS 133.0-B-2 Issue 2, including Editorial Change 2, Space Packet PDU profile**.
- A clause-level CCSDS Space Packet compliance matrix with Annex A PICS scope classification, implementation references, and named evidence.
- `PacketErrorControlMode::None` alongside the existing CRC16 default.
- `Packet::deserializeBounded()` overloads that parse one declared packet and report the consumed byte count.
- `Packet::getSerializedSize()` using `std::size_t`, covering the complete 65,542-octet Space Packet size range.
- Independent Python-generated CCSDS golden vectors committed as fixed test resources.
- Negative, parser-regression, rollover, Idle Packet, PVN, and maximum-size conformance tests.
- An installed-package external consumer that resolves CCSDSPack only through `find_package()` and validates version 1.2.0 on Linux and Windows.
- Encoder, decoder, and validator support for explicit `crc16` and `none` profiles.
- Decoder support for concatenated packets and preservation of unrelated trailing bytes.
- Detailed Doxygen contracts and representative usage documentation for every installed public header.
- Native Raspberry Pi 5 arm64 package validation and deterministic NUCLEO-H755ZI-Q Cortex-M7 hardware validation.

### Changed

- Packet Data Length now follows CCSDS semantics exactly: the number of Packet Data Field octets minus one.
- The optional CCSDSPack CRC16 value is documented and encoded as a mission-profile trailer inside the Packet Data Field, not as a third CCSDS structural field.
- CRC16 parsing validates the received trailer before committing decoded state.
- Packet parsing is bounded by the encoded Packet Data Length and no longer consumes unrelated trailing input.
- `Manager` sequence counts advance once per generated packet, use the full 14-bit range, and roll over modulo 16384.
- `Manager` binds one complete Packet Identification value: version, packet type, Secondary Header Flag, and APID.
- Public inspection getters no longer perform hidden packet mutation or finalization.
- The complete 11-bit APID range is supported; APID `0x7FF` is treated as Idle.
- Packet generation and configuration enforce Packet Version Number `000`.
- Idle Packets require Secondary Header Flag zero, no installed secondary header, and at least one mission-defined idle-data octet.
- The legacy 16-bit `getFullPacketLength()` remains source-compatible and saturates at `UINT16_MAX`; use `getSerializedSize()` for the exact complete range.
- Telemetry and telecommand packets both use Packet Sequence Count semantics. Telecommand Packet Name semantics are not implemented.

### Fixed

- Incorrect Packet Data Length generation and maximum-length handling.
- CRC coverage, extraction, and parse-time validation defects.
- Packet-boundary handling for concatenated and truncated input.
- Sequence-count coupling to sequence flags and inconsistent rollover behavior.
- Acceptance of mixed packet identifiers within one Manager stream.
- Partial state mutation after failed vector or buffer loads.
- Non-zero Packet Version Number generation and invalid Idle Packet structures.
- CLI diagnostics for distinct length, CRC, version, APID, and sequence failures.

### Compatibility

CCSDSPack 1.2.0 is intended to remain **source-compatible** with existing v1 public API usage. Additive APIs and overloads are used where the previous signature could not represent the corrected behavior.

The corrected wire format is not guaranteed to be compatible with packets produced by earlier releases. Stored or transmitted pre-1.2 packets may differ in Packet Data Length, CRC coverage, sequence behavior, packet boundaries, Packet Identification enforcement, PVN validation, or Idle Packet validation. Regenerate or explicitly migrate those packets before adopting the v1.2 profile.

CRC16 remains the default for existing v1 construction and configuration paths.

### Conformity scope

The release claim is limited to a **CCSDS 133.0-B-2 EC2 Space Packet PDU profile**. CCSDSPack does not implement the complete abstract Packet Service, Octet String Service, all protocol procedures, managed-parameter interfaces, or a completed Protocol Implementation Conformance Statement.

The bundled `PusA`, `PusB`, and `PusC` types remain legacy project-specific secondary-header formats. They are not official ECSS Packet Utilisation Standard implementations.

### Validation

The release was validated by:

- Ubuntu 22.04, Ubuntu 24.04, and `ubuntu-latest` native builds and tests;
- Windows `windows-latest` MinGW build and tests;
- CLI integration on Linux and Windows;
- installation and exact-version external-consumer checks on Linux and Windows;
- Ubuntu 22.04 native, Cortex-M, aarch64, and package generation;
- independent byte vectors and 93 native regression/conformance tests;
- Raspberry Pi 5 native arm64 package installation, CLI integration, and external consumer execution, ending in `CCSDSPACK_AARCH64_TEST:PASS`;
- NUCLEO-H755ZI-Q Cortex-M7 packet, CRC, Manager, Validator, PVN, and Idle Packet execution, with `CCSDSPACK_MCU_TEST:PASS` observed twice;
- promotion of the validated `develop` state to `main` through pull request #107.

Detailed target evidence is recorded in `docs/V1_2_HARDWARE_VALIDATION.md`. Clause-level protocol traceability is recorded in `CCSDS_COMPLIANCE.md`.

## Earlier releases

Changes for releases before 1.2.0 remain available in the repository commit history and GitHub Releases.

[1.2.0]: https://github.com/ExoSpaceLabs/CCSDSPack/compare/v1.1.1...v1.2.0
