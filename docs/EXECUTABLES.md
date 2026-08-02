<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Executables

[Documentation index](README.md) | [Canonical CLI reference](CLI.md)

CCSDSPack provides these host-side executables:

- `ccsds_encoder`: converts application bytes into one or more adjacent Space Packets;
- `ccsds_decoder`: parses adjacent Space Packets and reassembles application bytes;
- `ccsds_validator`: reports packet, identifier, CRC-profile, and sequence-stream failures;
- `CCSDSPack_tester`: runs the native regression and conformance test suite.

The authoritative option, packet-error-control, trailing-byte, validation, and exit-code documentation is maintained in [CLI.md](CLI.md). This page remains as a stable compatibility entry point for older links.

## Build controls

```bash
cmake -S . -B build \
  -DENABLE_ENCODER=ON \
  -DENABLE_DECODER=ON \
  -DENABLE_VALIDATOR=ON \
  -DENABLE_TESTER=ON
cmake --build build
```

The executable location depends on the selected generator and platform. The default project layout places native binaries under the configured build output's `bin` directory.

## Typical flow

```bash
ccsds_encoder -i payload.bin -o packets.bin -c template.cfg
ccsds_validator -i packets.bin -c template.cfg --verbose
ccsds_decoder -i packets.bin -o recovered.bin -c template.cfg
```

The encoder and decoder require a template configuration. The validator can operate without one, but a template enables Packet Identification checks. All tools must use the same `crc16` or `none` packet-error-control profile as the packet stream.

Run the installed or built test executable with:

```bash
CCSDSPack_tester
```

A non-zero process status indicates a failed command or test.
