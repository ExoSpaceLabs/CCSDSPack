# Command-line tools

CCSDSPack installs three host-side command-line programs:

- `ccsds_encoder` converts application bytes into one or more CCSDS Space Packets;
- `ccsds_decoder` consumes complete adjacent packets and reassembles application bytes;
- `ccsds_validator` reports parser, packet, secondary-header, and sequence-stream failures.

The tools construct the same complete `ccsds::Packet` template used by `ccsds::Manager`. There is no separate MissionProfile runtime object.

## Packet error control

All three programs accept:

```text
-e, --packet-error-control <crc16|none>
```

The command-line value overrides the packet-level `ccsds_packet_error_control` setting. This policy is independent of whether the packet uses no secondary header, a custom header, or PUS.

The receiver cannot infer whether the final two Packet Data Field bytes are the optional CCSDSPack CRC trailer. Decoder and validator must use the same mode as the encoder.

## Encoder

```bash
ccsds_encoder \
  --input payload.bin \
  --output packets.bin \
  --config template.cfg
```

The encoder calculates Packet Data Length from the complete Packet Data Field. In CRC16 mode, the two packet-error-control bytes contribute to the encoded length. CRC16 covers the serialized six-byte primary header followed by secondary-header and application bytes; it excludes the CRC bytes themselves.

CRC-free example:

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

The decoder walks the stream using each packet's encoded Packet Data Length. Adjacent packets are decoded in order. A suffix that cannot form another complete packet remains unconsumed rather than being appended to application data.

For PUS, the template's concrete secondary header is the parsing schema. Revision/direction come from `rev_a`/`rev_c` and `TcHeader`/`TmHeader`; optional layout choices such as CUC time come from the installed header tailoring. Manager clones that complete template contract for each packet.

Use `--trailing-output` to preserve an unrelated suffix:

```bash
ccsds_decoder -i framed-input.bin -o payload.bin -c template.cfg \
  --trailing-output trailing.bin
```

Without external framing, an arbitrary suffix that happens to form a syntactically complete CCSDS header cannot be distinguished from another packet. Use a synchronization marker or validate the complete stream when that ambiguity matters.

## Validator

```bash
ccsds_validator --input packets.bin --config template.cfg --verbose
```

The executable uses `ccsds::Packet` for bounded parsing and `ccsds::Validator` for structured object checks. The configured Packet template supplies the expected Packet Identification, packet-level PEC mode, and secondary-header contract.

Named checks can include:

- primary-header validity and Packet Version Number;
- Packet Data Length;
- CRC16 when packet error control is `crc16`;
- secondary-header presence and directional Packet Type consistency;
- sequence-flag state and modulo-16384 sequence-count continuity;
- Packet Identification and segmentation class against the template;
- template packet-error-control equality;
- template secondary-header type and PUS tailoring equality;
- PUS revision, direction, and Packet Type consistency;
- PUS tailoring and encoded secondary-header size;
- version/reserved and spare fields;
- telecommand acknowledgement flags and source ID;
- telemetry destination ID;
- PUS-A TM packet-subcounter state;
- PUS-C TM time-reference status;
- active CUC timestamp validity.

Only checks actually performed appear in the report. In `none` mode there is no `Crc16` entry.

Malformed input can fail during bounded parsing before a structured report exists. PUS parse failures are reported as `PUS secondary header : FAILED`; generic failures are reported as packet-parse failures.

`--print-packets` prints packets that parse successfully. The process returns exit code `18` when any packet or trailing stream bytes fail parsing or validation.

## Configuration

Template configuration is mandatory for encoder/decoder and optional for generic validator use.

Minimal generic example:

```ini
ccsds_packet_error_control:string=crc16
ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_APID:int=42
ccsds_segmented:bool=false
define_secondary_header:bool=false
```

PUS identity is selected by the concrete selector:

```ini
define_secondary_header:bool=true
secondary_header_type:string=PUS:revC:TC
```

The selector supplies revision and direction; optional PUS-A widths, PUS-A TM subcounter, CUC time, and spare-octet settings configure only actual tailoring.

See [CONFIG.md](CONFIG.md) and [`example/config`](../example/config).

The command-line programs and `ccsds::Config` are hosted-only. The underlying Packet, PUS header/tailoring, CUC time, raw-buffer, and Validator APIs remain available in the C++17 `CCSDS_MCU` static-library build.
