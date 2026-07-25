<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# CCSDSPack documentation

[Main README](../README.md) | [v1.2 compliance statement](../COMPLIANCE.md)

CCSDSPack is a C++17 library for creating, serializing, parsing, validating, and managing CCSDS Space Packet PDUs. Start with the examples, then use the profile and API reference for protocol details.

## Start here

- [Examples](EXAMPLES.md): current C++17 construction, segmentation, file I/O, parsing, CRC-free operation, and configuration examples.
- [Configuration reference](CONFIG.md): packet-template and command-line configuration keys.
- [Command-line tools](CLI.md): encoder, decoder, validator, packet-error-control modes, and exit behaviour.
- [Generated API reference](https://exospacelabs.github.io/CCSDSPack/html/): installed public types and functions.

## Compliance and behaviour

- [Concise v1.2 compliance statement](../COMPLIANCE.md): the supported release claim and its explicit boundary.
- [CCSDS 133.0-B-2 EC2 Space Packet PDU profile](CCSDS_133_0_B_2_PROFILE.md): detailed protocol scope, packet rules, limitations, and evidence.
- [v1.2 current behaviour](V1_2_CURRENT_BEHAVIOUR.md): implementation behaviour and compatibility notes.
- [Packet processing flow](FLOW.md): packet and Manager lifecycle from construction through parsing.

## Integration and delivery

- [Packages](PACKAGES.md): native packages and CMake package consumption.
- [Cross-build guide](CROSSBUILD.md): aarch64 Linux and bare-metal Cortex-M builds.
- [Legacy cross-compilation notes](CROSSCOMPILE.md): older environment-specific guidance retained for reference.
- [Container image](../docker/README.md): Docker build and runtime usage.

## Reference

- [Error and Result handling](ERROR.md): exception-free result types and error categories.
- [Executable overview](EXECUTABLES.md): compatibility entry point for the canonical CLI reference.

## Internal project notes

`PRIV_HELPER.md` documents the repository's historical GitFlow helper commands. It is maintainer guidance, not part of the public library API or compliance claim.
