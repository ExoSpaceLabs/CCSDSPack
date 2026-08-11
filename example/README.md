# CCSDSPack standalone examples

Each example is an independent CMake consumer of an installed CCSDSPack 2.x package. None links the source-tree target directly, so the suite also validates the installed headers, exported CMake target, and package surface.

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

The installation-prefix argument is optional when `CCSDSPACK_PREFIX` or `CMAKE_PREFIX_PATH` already identifies the package. Builds are written under `build/examples/<selection>` by default; `CCSDSPACK_EXAMPLE_BUILD_DIR` overrides that root.

Available examples:

- `basic_packet`: constructs and parses a generic CRC-free Space Packet with the vector API;
- `custom_secondary_header`: registers, serializes, and parses a mission-specific secondary header;
- `pus_c_telecommand`: constructs and parses a PUS-C telecommand using the concrete `rev_c::TcHeader` API;
- `pus_c_telemetry`: constructs and parses timestamped PUS-C telemetry with numeric CUC time;
- `raw_buffer_packet`: derives the declared packet size from six primary-header bytes and parses a pointer-plus-size receive buffer;
- `raw_buffer_manager`: uses raw application/stream buffers and const-reference Manager inspection APIs.

The raw-buffer examples deliberately consume only the installed public package. Linux and Windows CI build and execute the complete example set.

The `config/` directory contains complete Packet-template configurations for:

- generic CCSDS packets;
- PUS-A TC and TM;
- PUS-C TC;
- PUS-C TM with and without numeric CUC time.

`pus_c_tm.cfg` demonstrates the CCSDS 1958 TAI epoch, an explicit P-field, four coarse octets, and two fine octets. `pus_c_tm_no_time.cfg` demonstrates PUS-C telemetry without a timestamp field.

See [`docs/EXAMPLES.md`](../docs/EXAMPLES.md), [`docs/CONFIG.md`](../docs/CONFIG.md), and [`docs/RAW_BUFFERS.md`](../docs/RAW_BUFFERS.md).
