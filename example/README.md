# CCSDSPack standalone examples

Each example is an independent CMake consumer of an installed CCSDSPack package. None of them links the source-tree target directly.

Install the library, then build every example:

```bash
cmake -S . -B ../build
cmake --build ../build
cmake --install ../build --prefix ../build/install

./example/build_examples.sh all ../build/install
```

Build one example by directory name:

```bash
./example/build_examples.sh raw_buffer_packet ../build/install
./example/build_examples.sh raw_buffer_manager ../build/install
./example/build_examples.sh pus_c_telecommand ../build/install
```

The installation prefix argument is optional when `CCSDSPACK_PREFIX` or `CMAKE_PREFIX_PATH` already identifies the package. The script builds into `build/examples/<selection>` by default; override that root with `CCSDSPACK_EXAMPLE_BUILD_DIR`.

Available examples:

- `basic_packet`: creates and parses a generic CRC-free Space Packet using the vector convenience API.
- `custom_secondary_header`: registers, serializes, and parses a mission-specific header.
- `pus_c_telecommand`: creates and parses a PUS-C telecommand using `PUS:revC:TC`.
- `pus_c_telemetry`: creates and parses timestamped PUS-C telemetry using `PUS:revC:TM`.
- `raw_buffer_packet`: inspects the declared packet size from only six primary-header bytes and parses a pointer-plus-size receive buffer.
- `raw_buffer_manager`: uses raw application/stream buffers and const-reference Manager inspection APIs.

The raw-buffer examples deliberately consume only the installed public package. Their tests therefore verify that `CCSDSBuffer.h`, the Manager overloads, and the symbolic error names are exported correctly rather than succeeding because source-tree headers happen to be nearby. Linux and Windows CI build and execute the complete example set.

The `config/` directory contains complete profiles consumed by Packet, Manager,
and all three CLIs:

- `generic.cfg`;
- `pus_a_tc.cfg` and `pus_a_tm.cfg`;
- `pus_c_tc.cfg`, `pus_c_tm_no_time.cfg`, and `pus_c_tm.cfg`.

`pus_c_tm_no_time.cfg` demonstrates PUS-C telemetry without a time field.
`pus_c_tm.cfg` demonstrates numeric basic CUC time with the CCSDS 1958 TAI
epoch, an explicit P-field, four coarse octets, and two fine octets.

See [`docs/RAW_BUFFERS.md`](../docs/RAW_BUFFERS.md) for the current-copy versus future zero-copy boundary of the raw-buffer API.
