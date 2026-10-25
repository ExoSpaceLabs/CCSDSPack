<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack documentation

CCSDSPack v2.1 uses an authoritative C11 protocol core with a compatible C++17 ownership/convenience layer for creating, serializing, parsing, validating, and managing CCSDS Space Packet PDUs with PUS-A/PUS-C, numeric CUC time, packet-level CRC16, and hosted/embedded integration.

## Core documentation

- [Main README](../README.md): project overview, design, primary APIs, build, and integration model.
- [Standalone C11 API](C_API.md): public C facilities, ownership and target boundaries.
- [Examples](EXAMPLES.md): packet construction, PUS, Manager, validation, raw buffers, and configuration examples.
- [Space Packet PDU profile](CCSDS_133_0_B_2_PROFILE.md): supported CCSDS packet behavior and scope boundary.
- [PUS tailoring](MISSION_TAILORING.md): concrete PUS identities, optional layout choices, and numeric CUC time.
- [Structured validation](VALIDATION.md): named packet/template/PUS checks and sequence validation.
- [Incremental C11 stream receiver](STREAM_RECEIVER.md): chunked UART/DMA input, synchronization recovery, and bounded caller-owned storage.
- [Raw-buffer APIs](RAW_BUFFERS.md): transport-facing pointer-plus-size interfaces.
- [Performance baseline](PERFORMANCE.md): matched v2.0 `main` versus v2.1 packet, PUS, CUC, Validator, Manager, and CRC measurements, allocation accounting, C packet views, and MCU footprint.
- [Architecture](ARCHITECTURE.md): detailed C11/C++17 layering, ownership boundaries and build profiles.
- [Packet processing flow](FLOW.md): Packet, Manager, parsing, validation, and reassembly lifecycle.

## Hosted integration

- [Configuration reference](CONFIG.md): host-side Packet-template configuration schema.
- [Command-line tools](CLI.md): encoder, decoder, validator, and exit behavior.
- [Packages](PACKAGES.md): native packages and installed CMake consumption.
- [Cross-build guide](CROSSBUILD.md): aarch64 Linux and bare-metal Cortex-M builds.
- [Error and Result handling](ERROR.md): exception-free operation errors and structured validation diagnostics.
- [Robustness and fuzz coverage](ROBUSTNESS.md): sanitizer and bounded libFuzzer release gates.
- [Raspberry Pi 5 and STM32H755 validation](V2_HARDWARE_VALIDATION.md): current qualification gates and historical hardware evidence.
- [STM32H755 standalone test procedure](../test/package_tester/stm32h7xx/H755_INTEGRATION.md): build, DAS v0.1.0 test-only pin, flash and UART PASS markers.
- [Generated API reference](https://exospacelabs.github.io/CCSDSPack/html/): public headers and API details.

## Compliance

- [Concise compliance statement](../COMPLIANCE.md)
- [Detailed CCSDS Space Packet compliance matrix](../CCSDS_COMPLIANCE.md)
- [PUS/CUC compliance baseline](PUS_CUC_COMPLIANCE.md)
- [PUS-C independent evidence](PUS_C_EVIDENCE.md): acknowledgement vectors and conformance checks.
- [Structured validation evidence](VALIDATION_EVIDENCE.md): complete public validation-code traceability.

## Migration and historical references

Upgrade-specific source, configuration, package, CLI, and wire-format guidance is maintained exclusively in [Migrating CCSDSPack v1 to v2](MIGRATION_V1_TO_V2.md). Repository contribution and release workflow is documented in [CONTRIBUTING.md](../CONTRIBUTING.md).

Historical v1.2 behavior and hardware evidence remain available in the `V1_2_*` documents for release archaeology and regression reference; they do not define the v2 API.

## Diagrams

The main README includes a compact Mermaid architecture overview; the [detailed architecture](ARCHITECTURE.md) is also maintained as Mermaid source. The precision-sensitive packet-layout image remains in the README. UML generation is manual-only and is not a v2.1.0 release gate.
