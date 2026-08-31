<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack documentation

CCSDSPack is a C++17 library for creating, serializing, parsing, validating, and managing CCSDS Space Packet PDUs with optional PUS-A/PUS-C secondary headers, numeric CUC time, packet-level CRC16, and hosted/embedded integration.

## Core documentation

- [Main README](../README.md): project overview, design, primary APIs, build, and integration model.
- [Examples](EXAMPLES.md): packet construction, PUS, Manager, validation, raw buffers, and configuration examples.
- [Space Packet PDU profile](CCSDS_133_0_B_2_PROFILE.md): supported CCSDS packet behavior and scope boundary.
- [PUS tailoring](MISSION_TAILORING.md): concrete PUS identities, optional layout choices, and numeric CUC time.
- [Structured validation](VALIDATION.md): named packet/template/PUS checks and sequence validation.
- [Raw-buffer APIs](RAW_BUFFERS.md): transport-facing pointer-plus-size interfaces.
- [Packet processing flow](FLOW.md): Packet, Manager, parsing, validation, and reassembly lifecycle.

## Hosted integration

- [Configuration reference](CONFIG.md): host-side Packet-template configuration schema.
- [Command-line tools](CLI.md): encoder, decoder, validator, and exit behavior.
- [Executable overview](EXECUTABLES.md): hosted executable set and build controls.
- [Packages](PACKAGES.md): native packages and installed CMake consumption.
- [Cross-build guide](CROSSBUILD.md): aarch64 Linux and bare-metal Cortex-M builds.
- [Error and Result handling](ERROR.md): exception-free operation errors and structured validation diagnostics.
- [Generated API reference](https://exospacelabs.github.io/CCSDSPack/html/): public headers and API details.

## Compliance

- [Concise compliance statement](../COMPLIANCE.md)
- [Detailed CCSDS Space Packet compliance matrix](../CCSDS_COMPLIANCE.md)
- [PUS/CUC compliance baseline](CCSDS_COMPLIANCE.md)

## Migration and historical references

Upgrade-specific source, configuration, package, CLI, and wire-format guidance is maintained exclusively in [Migrating CCSDSPack v1 to v2](MIGRATION_V1_TO_V2.md).

Historical v1.2 behavior and hardware evidence remain available in the `V1_2_*` documents for release archaeology and regression reference; they do not define the v2 API.

`CROSSCOMPILE.md` is a compatibility pointer to the maintained cross-build guide. `PRIV_HELPER.md` contains maintainer workflow notes rather than public API documentation.

## Diagrams

The maintained packet-layout and architecture diagrams are embedded in the main README. UML generation is manual-only and is not a v2.0.0 release gate.
