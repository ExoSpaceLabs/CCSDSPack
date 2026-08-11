# Command-line tools

CCSDSPack installs three host-side command-line programs:

- `ccsds_encoder` converts application bytes into one or more CCSDS Space Packets;
- `ccsds_decoder` consumes complete adjacent packets and reassembles application bytes;
- `ccsds_validator` reports parser, packet, profile, and sequence-stream failures.

All tools load the same explicit v2 mission profile used by `ccsds::Packet` and
`ccsds::Manager`.

## Packet error control

All three programs accept:

```text
-e, --packet-error-control <crc16|none>
```

The command-line value overrides `ccsds_packet_error_control` for a generic
profile. For PUS, an override must match the profile because packet-error control
is part of the selected wire contract.

The receiver cannot infer whether the final two packet-data-field bytes are a CRC. The decoder and validator must therefore be configured with the same mode used by the encoder.

## Encoder

```bash
ccsds_encoder \
  --input payload.bin \
  --output packets.bin \
  --config template.cfg
```

The encoder calculates Packet Data Length from the complete packet data field. In CRC16 mode, the two packet-error-control bytes contribute to the encoded length. CRC16 covers the serialized six-byte primary header followed by the secondary header and application data; it excludes the CRC bytes themselves.

Generate CRC-free packets with an additive override:

```bash
ccsds_encoder -i payload.bin -o packets.bin -c template.cfg \
  --packet-error-control none
```

## Decoder

```bash
ccsds_decoder \
  --input packets.bin \
  --output payload.bin \
  --config template.cfg
```

The decoder walks the stream using each packet's encoded Packet Data Length. Adjacent packets are decoded in order. A suffix that cannot form another complete packet is left unconsumed rather than appended to application data.

For PUS profiles, Manager creates the selected revision/direction codec for every
packet and parses identifier, service, counter, and CUC time fields before
returning application data.

Use `--trailing-output` to preserve that suffix:

```bash
ccsds_decoder -i framed-input.bin -o payload.bin -c template.cfg \
  --trailing-output trailing.bin
```

Without external framing, an arbitrary suffix that happens to form a syntactically complete CCSDS header cannot be distinguished from another packet. Use a Manager synchronization marker or validate the complete stream when such ambiguity matters.

## Validator

```bash
ccsds_validator --input packets.bin --config template.cfg --verbose
```

The executable does not maintain a second protocol-validator implementation.
It uses `ccsds::Packet` for bounded parsing and `ccsds::Validator` for the
structured object/profile checks described in [VALIDATION.md](VALIDATION.md).

The library report can include named checks for:

- primary-header validity and Packet Version Number;
- Packet Data Length;
- CRC16 when the configured profile enables it;
- secondary-header presence;
- sequence-flag state and modulo-16384 sequence-count continuity;
- Packet Identification, segmentation class, and mission-profile equality when a template is supplied;
- PUS revision, direction, and CCSDS Packet Type consistency;
- PUS profile and secondary-header size;
- version/reserved and spare fields;
- telecommand acknowledgement flags and source ID;
- telemetry destination ID;
- PUS-A TM packet subcounter policy;
- PUS-C TM time-reference status;
- configured CUC timestamp validity.

Only checks that are actually performed are emitted by the structured report. In
`none` mode there is no `Crc16` report entry.

Malformed wire input can fail during bounded parsing before a structured
`ValidationReport` exists. PUS parse failures are reported as `PUS secondary
header : FAILED`; generic parse failures are reported as packet-parse failures.

`--print-packets` prints packets that parse successfully. The process returns exit code `18` when any packet or trailing stream bytes fail parsing or validation.

## Configuration

The template configuration is mandatory for the encoder and decoder and optional
for generic validator use. Every v2 template includes:

```ini
mission_profile:string=generic
ccsds_packet_error_control:string=crc16
```

Accepted configuration values are exact lowercase `crc16` and `none`. CLI option
values are case-insensitive. PUS profiles additionally declare their canonical
selector, revision, direction, identifiers, and time policy. See
[CONFIG.md](CONFIG.md) and the runnable profiles in
[`example/config`](../example/config).

The command-line programs and `ccsds::Config` are hosted-only components. The
underlying Packet, MissionProfile, PUS, time, and Validator APIs remain available
in the C++17 `CCSDS_MCU` static-library build.
