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
./example/build_examples.sh pus_c_telecommand ../build/install
```

The installation prefix argument is optional when `CCSDSPACK_PREFIX` or `CMAKE_PREFIX_PATH` already identifies the package. The script builds into `build/examples/<selection>` by default; override that root with `CCSDSPACK_EXAMPLE_BUILD_DIR`.

Available examples:

- `basic_packet`: creates and parses a generic CRC-free Space Packet.
- `custom_secondary_header`: registers, serializes, and parses a mission-specific header.
- `pus_c_telecommand`: creates and parses a PUS-C telecommand using `PUS:revC:TC`.
- `pus_c_telemetry`: creates and parses timestamped PUS-C telemetry using `PUS:revC:TM`.
