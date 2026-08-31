<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Executables

CCSDSPack hosted builds provide:

- `ccsds_encoder`: application data to adjacent Space Packets;
- `ccsds_decoder`: adjacent Space Packets to reassembled application data;
- `ccsds_validator`: bounded parsing plus structured packet/template/PUS validation;
- `CCSDSPack_tester`: native regression and conformance tests.

The canonical command-line option and behavior reference is [CLI.md](CLI.md).

## Build controls

```bash
cmake -S . -B build \
  -DENABLE_ENCODER=ON \
  -DENABLE_DECODER=ON \
  -DENABLE_VALIDATOR=ON \
  -DENABLE_TESTER=ON
cmake --build build
```

These executables are host-side components and are excluded when `CCSDSPACK_BUILD_MCU=ON`. The MCU static library retains Packet, Manager, PUS codecs/tailoring, CUC time, Result/Error, raw-buffer, and structured Validator APIs.

## Typical flow

```bash
ccsds_encoder -i payload.bin -o packets.bin -c template.cfg
ccsds_validator -i packets.bin -c template.cfg --verbose
ccsds_decoder -i packets.bin -o recovered.bin -c template.cfg
```

All tools use the same packet-error-control mode expected by the stream. A Validator template enables Packet Identification, segmentation class, PEC, and secondary-header contract checks.

Run the regression/conformance suite with:

```bash
CCSDSPack_tester
```

A non-zero process status indicates a failed operation or test.
