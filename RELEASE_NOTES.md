# CCSDSPack v2.0.0 release notes — draft

> [!IMPORTANT]
> These release notes describe the current v2.0.0 release candidate. Tagging remains blocked until the remaining fuzz/sanitizer, vector-traceability, arm64, STM32, and publication gates are complete.

## Summary

CCSDSPack v2.0.0 is a C++17 Space Packet library focused on deterministic packet construction, bounded parsing, explicit packet policy, standards-oriented PUS secondary headers, structured validation, and practical hosted/embedded integration.

The release provides a single Packet-centered ownership model: generic CCSDS state and packet error control belong to `ccsds::Packet`, concrete secondary headers own their wire layout, and `ccsds::Manager` uses a complete Packet template as its stream contract.

## Standards scope

- CCSDS 133.0-B-2 Issue 2, including Editorial Change 2, for the supported Space Packet PDU profile;
- ECSS-E-70-41A for supported PUS-A TC/TM secondary-header layouts;
- ECSS-E-ST-70-41C for supported PUS-C TC/TM secondary-header layouts;
- CCSDS 301.0-B-4 for the supported basic numeric CUC subset.

The release does not claim complete PUS services, transfer frames, COP-1, CFDP, a complete CCSDS protocol entity, UTC/leap-second conversion, or mission time correlation.

## Space Packet behavior

The packet layer provides:

- version-0 six-octet primary headers;
- full 11-bit APID support and Idle Packet structural validation;
- exact Packet Data Length calculation;
- bounded transactional parsing with consumed-byte reporting;
- modulo-16384 sequence handling and segmentation support;
- checked `Result`-based finalization and serialization;
- optional packet-level CRC-16/CCITT-FALSE or no packet error-control trailer;
- custom, opaque, and standards PUS secondary headers.

## PUS secondary headers

PUS-A/PUS-C TC/TM identity is represented by concrete types under `ccsds::pus::rev_a` and `ccsds::pus::rev_c`. Direction is intrinsic to `TcHeader` or `TmHeader`, and installing a directional header synchronizes the CCSDS Packet Type.

Direction-specific tailoring structs expose optional wire-layout choices. PUS-C TC source ID and TM destination ID remain fixed at two octets. PUS-A exposes supported optional identifier widths and the optional telemetry packet subcounter.

PUS parsing supports preinstalled-header schemas, typed `Packet::deserialize<HeaderT>()`, typed raw-buffer parsing, and canonical runtime selectors.

## Numeric CUC time

The basic CUC codec supports numeric coarse/fine counters, CCSDS-1958 or agency-defined epoch metadata, implicit/explicit basic P-field policy, 1–4 coarse octets, and 0–3 fine octets with overflow and P-field validation.

## Structured validation

`ccsds::Validator` returns a fixed-capacity `ValidationReport` with named checks for generic packet structure, sequence state, Packet-template contracts, PUS fields/tailoring, and active CUC timestamp state.

The report itself uses fixed `std::array` storage and performs no dynamic allocation. Validator remains available in `CCSDS_MCU` builds and does not require RTTI or exceptions.

## Raw transport interfaces

Vector APIs and pointer-plus-size transport APIs coexist. `ccsds::buffer::declaredPacketSize()` determines a complete packet boundary from the six-byte primary header, and bounded raw parsers support generic and typed PUS input. Manager provides raw application-data, packet, and stream overloads plus const-reference inspection APIs.

The v2.0.0 raw adapters currently bridge through vector-backed internals and therefore do not constitute a zero-copy or globally heap-free implementation.

## Manager

One `ccsds::Manager` represents one Packet Identification and one sequence stream. Its Packet template carries Packet Identification, packet-level PEC, concrete secondary-header type, and optional PUS tailoring. Manager provides segmentation, sequence assignment, stream serialization/parsing, transactional loading, and application-data reassembly.

## Hosted integration

Hosted builds provide encoder, decoder, validator, and regression-test executables; typed configuration files; an installed CMake package; standalone `find_package()` examples; Linux/Windows CI; Doxygen; and DEB/RPM/TGZ packaging support.

## Bare-metal integration

`CCSDSPACK_BUILD_MCU=ON` builds the protocol library as a C++17 static archive and excludes host-only configuration/CLI components. Packet, Manager, PUS codecs/tailoring, CUC time, Result/Error, raw-buffer adapters, and Validator remain available. Builds can use `-fno-exceptions -fno-rtti`.

No global heap-free claim is made for Packet/Manager/PUS storage.

## Validation status

The current candidate includes:

- 125 passing native regression/conformance tests;
- Linux Ubuntu 22.04, Ubuntu 24.04, and Ubuntu latest integration;
- Windows latest integration;
- Doxygen;
- CLI integration;
- installed shared-library consumer and standalone examples;
- Ubuntu 22.04 package/cross-build generation;
- Cortex-M compile/link coverage of the embedded public API;
- local ASan and UBSan runs.

Remaining release gates are tracked in the v2.0.0 milestone and release-readiness documents.

## Migration

Upgrade-specific source, configuration, CLI, package/SOVERSION, and wire-format guidance is maintained in [`docs/MIGRATION_V1_TO_V2.md`](docs/MIGRATION_V1_TO_V2.md).

## Release control

```text
v2.0.0-dev -> develop -> main -> tag v2.0.0
```

The `v2.0.0` tag is created only from an approved `main` commit after all release gates pass.
