# Cross-Build Guide (aarch64 Linux and Bare-metal MCU)

[Documentation index](README.md) | [Structured validation](VALIDATION.md)

CCSDSPack v2 is a C++17 library. This document covers:

- aarch64/arm64 Linux cross-builds;
- the bare-metal ARM Cortex-M static-library profile;
- `package.sh` package/archive generation.

## aarch64 Linux

The toolchain file `cmake/toolchains/aarch64-linux-gnu.cmake` is provided for
cross-compiling to arm64/aarch64.

### Ubuntu 22.04 and newer

Enable the foreign architecture and install the cross toolchain and runtime
libraries required by package dependency inspection:

```bash
sudo dpkg --add-architecture arm64
sudo apt-get update
sudo apt-get install -y \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static rsync \
  libc6:arm64 libgcc-s1:arm64 libstdc++6:arm64
```

Systems whose normal Ubuntu mirror does not serve arm64 must use the matching
`ports.ubuntu.com/ubuntu-ports` entries for that architecture.

Build and package:

```bash
./package.sh -p DEB -t cmake/toolchains/aarch64-linux-gnu.cmake
```

Build without packaging:

```bash
cmake -S . -B build-aarch64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake
cmake --build build-aarch64 -j
```

## Bare-metal MCU static library

`CCSDSPACK_BUILD_MCU=ON` builds the protocol library as a static archive and
defines `CCSDS_MCU` publicly. Host-only configuration parsing and command-line
executables are excluded.

The MCU library still contains the protocol-facing C++17 API, including:

- `ccsds::Packet` and `ccsds::Manager`;
- `ccsds::MissionProfile`;
- PUS-A/PUS-C TC/TM codecs;
- CUC time support;
- `ccsds::Result` error handling;
- `ccsds::Validator`, `ValidationCode`, and fixed-capacity `ValidationReport`.

The structured report uses `std::array` and performs no dynamic allocation
itself. The MCU build does not require RTTI or exceptions.

### Prerequisites

```bash
sudo apt update
sudo apt install -y \
  gcc-arm-none-eabi \
  binutils-arm-none-eabi \
  libnewlib-arm-none-eabi \
  libstdc++-arm-none-eabi-newlib
```

### Package a Cortex-M7 archive

```bash
./package.sh \
  -t cmake/toolchains/arm-none-eabi.cmake \
  -p MCU \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
```

`package.sh -m` forwards the flags to `CCSDSPACK_MCU_FLAGS`, so the flags apply
to the library itself as well as the MCU compile/link probe.

### Build only the static library

```bash
cmake -S . -B build-mcu \
  -DCMAKE_BUILD_TYPE=Release \
  -DCCSDSPACK_BUILD_MCU=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DCCSDSPACK_MCU_FLAGS="-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"

cmake --build build-mcu -j
```

`CCSDSPACK_MCU_FLAGS` is the current CMake cache variable for target-specific
MCU flags. Older documentation referring to `MCU_FLAGS` should not be used for
direct CMake configuration.

## Using the Validator on bare metal

Validation uses the same API as a hosted build:

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);

if (report.failed(ccsds::ValidationCode::PusDirection)) {
  // mission-specific fault handling
}
```

There is no dependency on `ccsds::Config`, file I/O, iostreams, exceptions, or
RTTI in the Validator API. The packet types themselves retain their existing
C++ containers and ownership model; the fixed-capacity statement applies to the
`ValidationReport`, not to every object in the library.

## `package.sh` reference

- `-p, --package-type`: `DEB`, `RPM`, `TGZ`, or `MCU`;
- `-t, --toolchain`: CMake toolchain file;
- `-m, --mcu-flags`: additional flags forwarded to the MCU library and probe;
- `-h, --help`: command usage.

Artifacts are written under `packages/`.

## Release validation

Cross-compilation proves that a target can be built; it is not a substitute for
execution on the target. v2 release acceptance records arm64 installed-package
execution and representative Cortex-M7 hardware execution separately under the
release-validation issue.

Historical v1.2 Raspberry Pi and STM32 results remain useful regression evidence
but are not silently treated as v2 hardware acceptance.
