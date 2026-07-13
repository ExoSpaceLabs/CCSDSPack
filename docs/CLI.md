# Command-line tools

CCSDSPack installs three host-side command-line programs:

- `ccsds_encoder` converts application bytes into one or more CCSDS Space Packets;
- `ccsds_decoder` consumes complete adjacent packets and reassembles application bytes;
- `ccsds_validator` reports packet-wire, identifier, and sequence-stream failures.

All existing v1 options remain supported. Options introduced for v1.2.0 are additive.

## Packet error control

All three programs accept:

```text
-e, --packet-error-control <crc16|none>
```

The command-line value overrides `ccsds_packet_error_control` from the configuration file. When neither is provided, the v1-compatible default is `crc16`.

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

The validator reports these checks separately:

- Packet Data Length and packet boundary;
- CRC16, or `NOT CHECKED` in `none` mode;
- CCSDS packet version;
- APID when a template config is supplied;
- packet type and secondary-header flag when a template config is supplied;
- sequence-flag state;
- sequence-count continuity, including rollover from 16383 to 0.

`--print-packets` prints packets which pass the parse-time length, CRC, and version checks. The process returns exit code `18` when any packet or trailing stream bytes fail validation.

## Configuration

The template configuration remains mandatory for the encoder and decoder and optional for the validator. The relevant v1.2 key is:

```ini
# Optional. Omit for the legacy-compatible CRC16 default.
ccsds_packet_error_control:string=crc16
```

Accepted values are `crc16` and `none`, case-insensitive in the CLI and accepted as `crc16`, `CRC16`, `none`, or `None` by the configuration loader.
