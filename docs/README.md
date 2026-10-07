<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack documentation

CCSDSPack v2.1 uses an authoritative C11 protocol core with a compatible C++17 ownership/convenience layer for creating, serializing, parsing, validating, and managing CCSDS Space Packet PDUs with PUS-A/PUS-C, numeric CUC time, packet-level CRC16, and hosted/embedded integration.

## Core documentation

- [Main README](../README.md): project overview, design, primary APIs, build, and integration model.
- [Examples](EXAMPLES.md): packet construction, PUS, Manager, validation, raw buffers, and configuration examples.
- [Space Packet PDU profile](CCSDS_133_0_B_2_PROFILE.md): supported CCSDS packet behavior and scope boundary.
- [PUS tailoring](MISSION_TAILORING.md): concrete PUS identities, optional layout choices, and numeric CUC time.
- [Structured validation](VALIDATION.md): named packet/template/PUS checks and sequence validation.
- [Raw-buffer APIs](RAW_BUFFERS.md): transport-facing pointer-plus-size interfaces.
- [Performance baseline](PERFORMANCE.md): matched v2.0 `main` versus v2.1 parse/serialization evidence, C zero-copy behavior, allocation accounting, and MCU footprint.
- [Packet processing flow](FLOW.md): Packet, Manager, parsing, validation, and reassembly lifecycle.

## Hosted integration

- [Configuration reference](CONFIG.md): host-side Packet-template configuration schema.
- [Command-line tools](CLI.md): encoder, decoder, validator, and exit behavior.
- [Packages](PACKAGES.md): native packages and installed CMake consumption.
- [Cross-build guide](CROSSBUILD.md): aarch64 Linux and bare-metal Cortex-M builds.
- [Error and Result handling](ERROR.md): exception-free operation errors and structured validation diagnostics.
- [Generated API reference](https://exospacelabs.github.io/CCSDSPack/html/): public headers and API details.

## Compliance

- [Concise compliance statement](../COMPLIANCE.md)
- [Detailed CCSDS Space Packet compliance matrix](../CCSDS_COMPLIANCE.md)
- [PUS/CUC compliance baseline](PUS_CUC_COMPLIANCE.md)

## Migration and historical references

Upgrade-specific source, configuration, package, CLI, and wire-format guidance is maintained exclusively in [Migrating CCSDSPack v1 to v2](MIGRATION_V1_TO_V2.md). Repository contribution and release workflow is documented in [CONTRIBUTING.md](../CONTRIBUTING.md).

Historical v1.2 behavior and hardware evidence remain available in the `V1_2_*` documents for release archaeology and regression reference; they do not define the v2 API.

## Diagrams

The maintained packet-layout and architecture diagrams are embedded in the main README. UML generation is manual-only and is not a v2.1.0 release gate.
