<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack documentation

[Main README](../README.md) | [v2 compliance baseline](CCSDS_COMPLIANCE.md) | [v1 to v2 migration](MIGRATION_V1_TO_V2.md)

CCSDSPack is a C++17 library for creating, serializing, parsing, validating, and managing CCSDS Space Packet PDUs. Start with the examples, then use the profile and API reference for protocol details.

## Start here

- [Examples](EXAMPLES.md): current C++17 construction, segmentation, file I/O, parsing, CRC-free operation, and configuration examples.
- [Configuration reference](CONFIG.md): packet-template and command-line configuration keys.
- [Command-line tools](CLI.md): encoder, decoder, validator, packet-error-control modes, and exit behaviour.
- [Generated API reference](https://exospacelabs.github.io/CCSDSPack/html/): installed public types and functions.
- [PUS mission tailoring](MISSION_TAILORING.md): explicit PUS-A/PUS-C revision, direction, identifier, time, spare, and packet-error-control choices.
- [v1 to v2 migration](MIGRATION_V1_TO_V2.md): removed legacy types and replacement APIs.

## Compliance and behaviour

- [v2 compliance baseline](CCSDS_COMPLIANCE.md): current PUS and inherited generic packet scope.
- [Concise v1.2 compliance statement](../COMPLIANCE.md): historical generic packet release claim.
- [CCSDS Space Packet compliance matrix](../CCSDS_COMPLIANCE.md): clause-level traceability, PICS scope classification, implementation references, and evidence.
- [CCSDS 133.0-B-2 EC2 Space Packet PDU profile](CCSDS_133_0_B_2_PROFILE.md): detailed protocol scope, packet rules, limitations, and evidence.
- [v1.2 current behaviour](V1_2_CURRENT_BEHAVIOUR.md): implementation behaviour and compatibility notes.
- [Packet processing flow](FLOW.md): packet and Manager lifecycle from construction through parsing.

## Integration and delivery

- [Packages](PACKAGES.md): native packages and CMake package consumption.
- [v1.2 hardware validation](V1_2_HARDWARE_VALIDATION.md): accepted Raspberry Pi arm64 and STM32H755 CM7 evidence.
- [STM32H755 validation procedure](V1_2_STM32_VALIDATION_STEPS.md): exact build, flash, UART, and acceptance steps used for the MCU release gate.
- [Cross-build guide](CROSSBUILD.md): aarch64 Linux and bare-metal Cortex-M builds.
- [Legacy cross-compilation notes](CROSSCOMPILE.md): older environment-specific guidance retained for reference.
- [Container image](../docker/README.md): Docker build and runtime usage.

## Reference

- [Error and Result handling](ERROR.md): exception-free result types and error categories.
- [Executable overview](EXECUTABLES.md): compatibility entry point for the canonical CLI reference.

## Internal project notes

`PRIV_HELPER.md` documents the repository's historical GitFlow helper commands. It is maintainer guidance, not part of the public library API or compliance claim.
