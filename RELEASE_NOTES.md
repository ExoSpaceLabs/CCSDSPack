# CCSDSPack v2.0.0 release notes — draft

> [!IMPORTANT]
> These are **draft** release notes for the v2.0.0 release candidate. v2.0.0 is not approved for tagging until the remaining conformance, fuzz/sanitizer, arm64, STM32, migration-guide, and publication gates are complete.

## Summary

CCSDSPack v2 is a breaking C++17 release that keeps the corrected v1.2 generic CCSDS Space Packet wire behavior and replaces the legacy project-specific PUS model with standards-oriented PUS-A and PUS-C revision/direction codecs.

The release also adds explicit mission profiles, numeric CUC time, checked packet finalization, structured packet validation, raw transport-buffer adapters, a lowercase public namespace, installed-package examples, and updated hosted/bare-metal integration.

## Standards scope

Implemented and documented baselines:

- CCSDS 133.0-B-2 Issue 2, including Editorial Change 2, for the supported Space Packet PDU profile;
- ECSS-E-70-41A for supported PUS-A TC/TM secondary-header layouts;
- ECSS-E-ST-70-41C for supported PUS-C TC/TM secondary-header layouts;
- CCSDS 301.0-B-4 basic CUC numeric time for the documented supported subset.

This is not a claim to implement every PUS service, transfer frames, COP-1, CFDP, a complete CCSDS protocol entity, UTC/leap-second conversion, or mission time correlation.

## Breaking API changes

- public namespace changes from `CCSDS` to `ccsds`;
- legacy project-specific `PusA`, `PusB`, and `PusC` classes are removed;
- `PusServices` is removed;
- there is no standards-facing PUS-B revision;
- PUS-A and PUS-C TC/TM use distinct public types under `ccsds::pus::rev_a` and `ccsds::pus::rev_c`;
- secondary-header APIs use `setSecondaryHeader`, `getSecondaryHeader`, and related CCSDS terminology rather than `DataFieldHeader` naming;
- packet finalization and serialization paths return checked `Result` types;
- the Validator no longer exposes a positional six-boolean report;
- hosted configuration uses the explicit v2 mission-profile schema and rejects legacy PUS selectors/keys.

## Raw transport-buffer APIs

The existing vector APIs remain available. v2.0.0 additionally exposes pointer-plus-size entry points for transport-owned buffers.

`ccsds::buffer::declaredPacketSize()` determines the complete Space Packet size from only the six-byte primary header, allowing UART, DMA, SpaceWire, TCP, or other receivers to determine how many additional bytes belong to one packet before the body is available.

`ccsds::buffer` also provides raw generic, PUS/custom typed, and opaque-header Packet parsing adapters. `ccsds::Manager` provides raw pointer + size overloads for application-data input, one-packet ingestion, and concatenated packet streams.

Const Manager objects additionally expose read-only references to the stored template, packet collection, and Validator, avoiding unnecessary copies during inspection. `ccsds::errorCodeName()` provides allocation-free symbolic labels for `ErrorCode` values.

The v2.0.0 raw parsing adapters currently bridge to vector-backed internals. The public API is intentionally structured so later zero-copy or heap-free parser work can be introduced without forcing callers to change these transport-facing signatures. v2.0.0 therefore does not claim a globally heap-free runtime.

## Structured validation

`ccsds::Validator::validate()` returns a `ccsds::ValidationReport` containing named `ValidationCode` checks.

The report covers the applicable subset of:

- primary-header/version validity;
- Packet Data Length;
- CRC16 when enabled;
- secondary-header presence;
- segmentation state and sequence-count continuity;
- Packet Identification, segmentation class, and mission-profile template matching;
- PUS revision, direction, and Packet Type consistency;
- PUS profile and encoded header size;
- PUS version/reserved bits and spare fields;
- telecommand acknowledgement flags and source ID;
- telemetry destination ID;
- PUS-A TM packet-subcounter policy;
- PUS-C TM four-bit time-reference status;
- configured CUC timestamp fit.

`ValidationReport` stores its checks in a fixed `std::array`, performs no dynamic allocation itself, requires only C++17, and remains available in `CCSDS_MCU` builds with exceptions and RTTI disabled.

The hosted `ccsds_validator` command delegates packet/profile checks to the library Validator rather than maintaining a parallel validation implementation.

## Mission profiles and PUS

A default `ccsds::MissionProfile` represents generic CCSDS without implicit PUS.

PUS profiles explicitly select:

- PUS-A or PUS-C;
- telecommand or telemetry direction;
- packet error control;
- applicable source/destination identifier widths;
- optional PUS-A TM packet subcounter;
- zero spare-octet count;
- optional CUC telemetry time and its epoch/P-field/coarse/fine layout.

Canonical selectors are:

- `PUS:revA:TC`;
- `PUS:revA:TM`;
- `PUS:revC:TC`;
- `PUS:revC:TM`.

## CUC time

The v2 CUC codec stores numeric coarse/fine counters with an explicit wire profile. The supported subset includes:

- CCSDS 1958 TAI or agency-defined epoch metadata;
- implicit or explicit basic one-octet P-field;
- 1–4 coarse octets;
- 0–3 fine octets;
- counter-width and P-field validation.

Calendar conversion, leap-second handling, and time correlation remain outside scope.

## Generic Space Packet behavior retained from v1.2

The following are **not new v2 wire changes**. They were corrected in v1.2 and remain the v2 foundation:

- correct Packet Data Length calculation;
- optional CRC16 coverage over primary header plus packet data excluding the CRC bytes themselves;
- bounded parsing and exact packet consumption;
- complete 11-bit APID support and Idle Packet handling;
- modulo-16384 Packet Sequence Count behavior;
- complete Packet Identification binding in Manager;
- non-mutating inspection getters.

## Hosted integration

v2 provides:

- encoder, decoder, and validator CLIs using the same mission-profile model;
- generic and PUS configuration examples;
- six standalone installed-package CMake consumers, including raw Packet and raw Manager examples;
- Linux Ubuntu 22.04, Ubuntu 24.04, and Ubuntu latest CI;
- Windows latest CI;
- Doxygen API documentation.

Automatic UML generation is intentionally disabled for v2.0.0 development. The workflow remains available manually through `workflow_dispatch`; diagrams are not a release gate.

## Bare-metal integration

The project remains C++17. `CCSDSPACK_BUILD_MCU=ON` builds the protocol library as a static archive and excludes hosted Config/CLI components.

The MCU public library includes Packet, Manager, MissionProfile, PUS-A/PUS-C codecs, CUC time, Result/Error, raw-buffer adapters, and the structured Validator.

The ARM compile/link consumer probe uses the same `CCSDSPACK_MCU_FLAGS` as the library and exercises the structured Validator, a representative PUS-C telecommand packet, declared packet-size inspection, and raw bounded Packet parsing.

## Validation status

Current development evidence includes:

- 115 hosted native tests after the structured Validator and raw-buffer work;
- Linux hosted builds across Ubuntu 22.04, 24.04, and latest;
- Windows hosted build;
- Doxygen;
- representative generic/PUS CLI integration;
- installed-package example execution including the raw-buffer examples;
- local ASan and UBSan runs;
- independent generic and representative PUS byte vectors;
- MCU compile/link probe coverage designed for `-fno-exceptions -fno-rtti`.

The final release still requires the remaining gates tracked by the v2 milestone, including dedicated fuzz/sanitizer CI, final PUS-C/negative traceability, fresh arm64 execution, fresh NUCLEO-H755ZI-Q/Cortex-M7 execution, final migration-guide approval, and release publication checks.

## Migration

See [`docs/MIGRATION_V1_TO_V2.md`](docs/MIGRATION_V1_TO_V2.md) for source, configuration, Validator, raw-buffer, and wire-layout migration guidance.

## Release control

The intended promotion path is:

```text
feature/* -> v2.0.0-dev -> develop -> main -> tag v2.0.0
```

The `v2.0.0` tag is created only after the final `main` commit passes the approved release gates.
