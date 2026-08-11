<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Executables

[Documentation index](README.md) | [Canonical CLI reference](CLI.md) | [Structured validation](VALIDATION.md)

CCSDSPack provides these host-side executables:

- `ccsds_encoder`: converts application bytes into one or more adjacent Space Packets;
- `ccsds_decoder`: parses adjacent Space Packets and reassembles application bytes;
- `ccsds_validator`: parses packet streams and reports the named checks produced by the library Validator;
- `CCSDSPack_tester`: runs the native regression and conformance test suite.

The authoritative option, packet-error-control, trailing-byte, validation, and exit-code documentation is maintained in [CLI.md](CLI.md). The Validator API itself is documented in [VALIDATION.md](VALIDATION.md).

## Build controls

```bash
cmake -S . -B build \
  -DENABLE_ENCODER=ON \
  -DENABLE_DECODER=ON \
  -DENABLE_VALIDATOR=ON \
  -DENABLE_TESTER=ON
cmake --build build
```

These executables are hosted components and are not built when
`CCSDSPACK_BUILD_MCU=ON`. The underlying Packet, Manager, MissionProfile, PUS,
time, Result, and structured Validator APIs remain available in the MCU static
library.

## Typical flow

```bash
ccsds_encoder -i payload.bin -o packets.bin -c template.cfg
ccsds_validator -i packets.bin -c template.cfg --verbose
ccsds_decoder -i packets.bin -o recovered.bin -c template.cfg
```

The encoder and decoder require a template configuration. The validator can operate without one for generic packets, but a template enables Packet Identification, segmentation-class, and mission-profile comparison. All tools must use the same `crc16` or `none` packet-error-control profile as the packet stream.

The validator executable delegates protocol/profile checks to `ccsds::Validator`; it does not maintain an independent copy of the validation rules.

Run the installed or built test executable with:

```bash
CCSDSPack_tester
```

A non-zero process status indicates a failed command or test.
