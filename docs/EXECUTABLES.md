# Executables

[../](README.md) - CCSDSPack Documentation

# Table of Contents

- [Common Conventions](#common-conventions)
- [Encoder](#encoder)
- [Decoder](#decoder)
- [Validator](#validator)
- [Tester](#tester)
- [Tips](#tips)
- [See Also](#see-also)


# Overview

CCSDSPack ships three small CLI tools built on top of the library:

- **`ccsds_encoder`** — encode a file into CCSDS packets and write them to a binary container.
- **`ccsds_decoder`** — read a binary container of CCSDS packets and reconstruct the original file.
- **`ccsds_validator`** — validate a packet container (and optionally check coherence against a template).

> **Build toggles (CMake)**:  
> `-DENABLE_ENCODER=ON -DENABLE_DECODER=ON -DENABLE_VALIDATOR=ON`

All three tools support a simple **configuration file** that carries the packet template and settings.  
The config parser supports `string|int|float|bool|bytes` types. For `int`, hex (e.g., `0x1F`) is supported; byte arrays can be defined as `[1, 2, 0xFF]`.

---

## Common Conventions

- **Exit codes**: `0` on success; non-zero on error (mirrors the library’s error-first contract).
- **Config file** (`-c/--config`): recommended to use the same config for encode/decode/validate for consistency.
- **Binary container format**: encoder/decoder/validator use the Manager’s built-in read/write helpers for consistent round-tripping.

---

## Encoder

Encode application-data bytes into one or more CCSDS packets and save the serialized packet stream as a binary file.

Usage:
```bash

./ccsds_encoder -i <file_to_encode> -o <packets.bin> -c <config.txt>
```

Options

| Flag                 | Description                                                |
|----------------------|------------------------------------------------------------|
| `-i, --input <path>`  | Path of the file to encode.                               |
| `-o, --output <path>` | Output file containing serialized CCSDS packets (binary). |
| `-c, --config <path>` | Configuration file with packet template/settings.         |
| `-h, --help`              | Show help and exit.                                   |
| `-v, --verbose`           | Show generated packets information               |

Example: Encode a firmware blob into CCSDS packets.
```bash

ccsds_encoder -i ./fw.bin -o ./fw_packets.bin -c ./template.cfg
```

## Decoder
Decode a previously encoded binary container of CCSDS packets back into the original data.

Usage:
```bash

ccsds_decoder -i <packets.bin> -o <output_file> -c <config.txt>
```
Options:

| Flag                 | Description                                                 |
|----------------------|-------------------------------------------------------------|
| `-i, --input <path>`  | Input binary container with serialized CCSDS packets.       |
| `-o, --output <path>` | Path to write the recovered application data.               |
| `-c, --config <path>` | Configuration file (ideally the same used during encoding). |
| `-h, --help`              | Show help and exit.                                         |
| `-v, --verbose`           | Show decoded packets information.                           |

---

Example: Decode the container back to bytes.
```bash

ccsds_decoder -i ./fw_packets.bin -o ./fw_out.bin -c ./template.cfg
```
## Validator
Validate a binary packet container (checks integrity/coherence, optionally against a template).

Usage:
```bash

ccsds_validator -i <packets.bin> -c <config.txt> [<flags>...]
```
Options:

| Flag                     | Description                                                                 |
|--------------------------|-----------------------------------------------------------------------------|
| `-i, --input <filename>`  | **Mandatory**: input file holding CCSDS packets to be validated.           |
| `-c, --config <filename>` | Configuration file; if provided, packets are validated against this template packet. |
| `-h, --help`              | Show help and exit.                                                        |
| `-v, --verbose`           | Show packet-specific validation report.                                    |
| `-p, --print-packets`     | Show information for each loaded packet.                                |

### Validate a packet file using your mission template
```bash

ccsds_validator -i ./fw_packets.bin -c ./template.cfg
```

## Tester
The CCSDSPack test suite will be built along with the library.

The test suite is designed for regression testing. It verifies:
* Packet encoding/decoding: Ensures that application data is correctly segmented into CCSDS
  packets and reassembled without corruption.

* Binary I/O: Confirms that Manager can write packets to a binary container and read them back
  to an identical in-memory representation.

* Configuration file parsing: Checks that packet templates and settings are loaded correctly from
  config files.

* Validator integration: Tests packet integrity checks, both with and without a template config.

* Error handling: Exercises error paths (invalid config, truncated packets, wrong APID, etc.) to 
  ensure consistent res.has_value() / res.error() behavior.

Running the tests
After building with BUILD_TESTER=ON:

```bash

./ccsdspack_tester
```

A non-zero exit code indicates a regression or failed check.
The tests are safe to run repeatedly and can be integrated into CI pipelines to automatically catch any behavior changes.

# Tips
Keep input/output paths distinct to avoid accidental overwrites.

Always use the same -c config during encode → validate → decode to prevent mismatches.

In automated pipelines or CI, run the validator before decoding to catch malformed or corrupt files early.

See Also

[Main-README](../README.md) – main overview, build instructions, and examples.

[API docs](https://exospacelabs.github.io/CCSDSPack/html/) – detailed reference published via GitHub Pages.
