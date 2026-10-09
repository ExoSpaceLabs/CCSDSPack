# Cross-build guide

[Documentation index](README.md) | [Packages](PACKAGES.md) | [Structured validation](VALIDATION.md)

CCSDSPack v2.1 has a C11 protocol core with an optional C++17 ownership/convenience layer and supports native hosted systems, aarch64 Linux targets, and bare-metal ARM Cortex-M consumers.

## aarch64 Linux

The repository provides `cmake/toolchains/aarch64-linux-gnu.cmake`.

Install the cross toolchain and target runtime dependencies required by package inspection. The repository helper `install-aarch-cross-build-deps.sh` is intentionally restricted to Ubuntu 22.04 (Jammy), because it configures Jammy amd64/arm64 package sources explicitly; other host releases should use their native repository layout:

```bash
sudo dpkg --add-architecture arm64
sudo apt-get update
sudo apt-get install -y \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static rsync \
  libc6:arm64 libgcc-s1:arm64 libstdc++6:arm64
```

Build directly:

```bash
cmake -S . -B build-aarch64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake
cmake --build build-aarch64 -j
```

Generate a package:

```bash
./package.sh -p DEB -t cmake/toolchains/aarch64-linux-gnu.cmake
```

## Bare-metal Cortex-M static library

`CCSDSPACK_BUILD_MCU=ON` builds static protocol libraries and defines `CCSDS_MCU` publicly. The C11 core is always available; the compatible C++17 archive is included when `CCSDSPACK_BUILD_CPP=ON`. Host-side configuration parsing and command-line executables are excluded.

The C11 MCU core retains allocation-free wire primitives, packet views, PUS codecs, CUC, validation, segmentation/reassembly, and stream framing. With the default C++ layer enabled, the MCU library also retains:

- `ccsds::Packet` and `ccsds::Manager`;
- PUS-A/PUS-C TC/TM codecs and tailoring types;
- numeric CUC time;
- `ccsds::Result` / `ccsds::Error`;
- raw-buffer adapters;
- `ccsds::Validator`, `ValidationCode`, and fixed-capacity `ValidationReport`.

A typical Cortex-M7 package build is:

```bash
./package.sh \
  -t cmake/toolchains/arm-none-eabi.cmake \
  -p MCU \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
```

Direct CMake configuration uses `CCSDSPACK_MCU_FLAGS`:

```bash
cmake -S . -B build-mcu \
  -DCMAKE_BUILD_TYPE=Release \
  -DCCSDSPACK_BUILD_MCU=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DCCSDSPACK_MCU_FLAGS="-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
cmake --build build-mcu -j
```

The fixed-capacity/no-allocation guarantee applies to `ValidationReport`; Packet, Manager, PUS, and parsing internals continue to use their documented C++ containers and ownership model.

## Package script

`package.sh` supports:

- `-p, --package-type`: `DEB`, `RPM`, `TGZ`, or `MCU`;
- `-t, --toolchain`: CMake toolchain file;
- `-m, --mcu-flags`: flags forwarded to the MCU library and compile/link probe;
- `-h, --help`: usage.

Artifacts are written under `packages/`.

## Validation boundary

Cross-compilation proves build/link compatibility for the target ABI. Release acceptance records native arm64 execution and representative Cortex-M7 hardware execution separately.
