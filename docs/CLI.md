# Command-line tools

CCSDSPack provides three host-side command-line programs:

- `ccsds_encoder` converts application bytes into one or more CCSDS Space Packets;
- `ccsds_decoder` parses adjacent packets and reassembles application bytes;
- `ccsds_validator` reports parser, packet, secondary-header, template, and sequence-stream failures.

The tools build and use the same complete `ccsds::Packet` template used by `ccsds::Manager`.

## Packet error control

All three tools accept:

```text
-e, --packet-error-control <crc16|none>
```

The command-line value overrides `ccsds_packet_error_control` from configuration. The selected mode applies to the enclosing Packet independently of its secondary-header type. Decoder and validator must use the mode expected by the packet stream.

## Encoder

```bash
ccsds_encoder --input payload.bin --output packets.bin --config template.cfg
```

Encoder calculates Packet Data Length from serialized content. In CRC16 mode, the two CRC bytes contribute to Packet Data Length and CRC coverage excludes the CRC bytes themselves.

## Decoder

```bash
ccsds_decoder --input packets.bin --output payload.bin --config template.cfg
```

Decoder walks adjacent packets using each encoded Packet Data Length. The template supplies packet PEC and the expected secondary-header schema/tailoring. A suffix that does not form a complete packet remains outside the decoded application data; `--trailing-output` can preserve it.

## Validator

```bash
ccsds_validator --input packets.bin --config template.cfg --verbose
```

Validator uses bounded Packet parsing followed by `ccsds::Validator`. The template supplies Packet Identification, packet-level PEC, and secondary-header contract.

Named checks can include primary-header/version, Packet Data Length, CRC16, secondary-header presence/direction, segmentation/sequence continuity, Packet Identification, template PEC/header equality, PUS revision/direction/tailoring, reserved/spare fields, TC acknowledgement/source ID, TM destination ID, PUS-A TM subcounter, PUS-C TM time-reference status, and CUC timestamp validity.

Only checks actually performed appear in the report. Malformed wire input can fail during bounded parsing before a structured Packet report exists.

`--print-packets` prints successfully parsed packets. Exit code `18` indicates packet/trailing-stream parsing or validation failure.

## Configuration

Encoder and decoder require a Packet-template configuration. Validator can operate without a template for generic packets, while a template enables identifier, PEC, segmentation-class, and secondary-header contract checks.

PUS identity is selected by a canonical concrete selector:

```ini
define_secondary_header:bool=true
secondary_header_type:string=PUS:revC:TC
```

Optional PUS and CUC keys configure only fields/tailoring supported by that concrete header.

See [CONFIG.md](CONFIG.md).

The command-line tools and `ccsds::Config` are hosted-only. The underlying Packet, Manager, PUS, CUC, Result/Error, raw-buffer, and Validator APIs remain available in the C++17 `CCSDS_MCU` library.
