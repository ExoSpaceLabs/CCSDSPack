# Cross-Build Guide (aarch64 Linux and Bare-metal MCU)

[../](README.md) - CCSDSPack Documentation

This document collects everything you need to cross-build CCSDSPack for:
- aarch64/arm64 Linux (e.g., Raspberry Pi 4/5)
- Bare‑metal static library for ARM Cortex‑M (e.g., STM32, M4/M7)

It also shows how to use the `package.sh` helper to generate packages or archives.

---
## Contents
- [aarch64 (arm64) cross-build for Linux](#aarch64-arm64-cross-build-for-linux)
  - [Prerequisites on Ubuntu 22.04 (Jammy) and newer](#prerequisites-on-ubuntu-2204-jammy-and-newer)
  - [Prerequisites on Ubuntu 20.04 (Focal)](#prerequisites-on-ubuntu-2004-focal)
  - [Build and package for aarch64](#build-and-package-for-aarch64)
- [Bare‑metal (MCU) static library](#bare-metal-mcu-static-library)
  - [Prerequisites](#prerequisites)
  - [Build and package for MCU](#build-and-package-for-mcu)
- [package.sh reference](#packagesh-reference)
- [Troubleshooting](#troubleshooting)

---
## aarch64 (arm64) cross-build for Linux

The toolchain file `cmake/toolchains/aarch64-linux-gnu.cmake` is provided for cross-compiling to arm64/aarch64.

### Prerequisites on Ubuntu 22.04 (Jammy) and newer
On Jammy-based systems (including GitHub Actions ubuntu-22.04 runners), it is usually enough to enable the foreign architecture and install the toolchain + runtime libraries used by `cpack`'s dependency scanner:

```bash
sudo dpkg --add-architecture arm64
sudo apt-get update
sudo apt-get install -y \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static rsync \
  libc6:arm64 libgcc-s1:arm64 libstdc++6:arm64
```

### Prerequisites on Ubuntu 20.04 (Focal)
On Focal, arm64 runtime packages are reliably served via `ports.ubuntu.com`. Configure a clean dual-arch sources list:

```bash
# 1) add arm64 architecture
sudo dpkg --add-architecture arm64
sudo dpkg --print-foreign-architectures  # should show: arm64

# 2) Backup current sources
sudo cp /etc/apt/sources.list /etc/apt/sources.list.bak.$(date +%s)

# 3) Write a clean dual-arch sources.list for Focal
sudo tee /etc/apt/sources.list >/dev/null <<'EOF'
# amd64 from main archive (Focal)
deb [arch=amd64] http://archive.ubuntu.com/ubuntu focal main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu focal-updates main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu focal-backports main restricted universe multiverse
deb [arch=amd64] http://security.ubuntu.com/ubuntu focal-security main restricted universe multiverse

# arm64 from ports (Focal)
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports focal main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports focal-updates main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports focal-backports main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports focal-security main restricted universe multiverse
EOF

# 4) Clean and refresh indexes
sudo apt-get clean
sudo rm -rf /var/lib/apt/lists/*
sudo apt-get -o Acquire::ForceIPv4=true update

# 5) Install toolchain and the arm64 runtime libs
sudo apt-get install -y \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static rsync \
  libc6:arm64 libgcc-s1:arm64 libstdc++6:arm64

# 6) Sanity checks
dpkg -L libc6:arm64      | grep -E 'ld-linux-aarch64\.so\.1|libc\.so\.6' || true
dpkg -L libgcc-s1:arm64  | grep libgcc_s.so.1 || true
dpkg -L libstdc++6:arm64 | grep 'libstdc\+\+\.so\.6' || true
```

### Build and package for aarch64
Use the provided toolchain file and `package.sh`:

```bash
# From the project root
./package.sh -p DEB -t cmake/toolchains/aarch64-linux-gnu.cmake

# Artifacts will be placed under ./packages/
ls -l packages/
```

If you prefer to just build (no packaging), configure and build in your usual build directory:

```bash
mkdir -p build-aarch64 && cd build-aarch64
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/aarch64-linux-gnu.cmake
cmake --build . -- -j
```

---
## Bare‑metal (MCU) static library

You can build a static library suitable for microcontrollers (exceptionless, no executables) by enabling `CCSDSPACK_BUILD_MCU` and using the `arm-none-eabi` toolchain.

### Prerequisites

```bash
sudo apt update
sudo apt install -y \
  gcc-arm-none-eabi \
  binutils-arm-none-eabi \
  libnewlib-arm-none-eabi \
  libstdc++-arm-none-eabi-newlib
```

### Build and package for MCU
Example for Cortex‑M7 (adjust flags for M4, etc.):

```bash
# Package a TGZ archive with headers + static library
./package.sh -t cmake/toolchains/arm-none-eabi.cmake -p MCU \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"

# Artifacts will be placed under ./packages/
ls -l packages/
```

If you only want to build the static library:

```bash
mkdir -p build-mcu && cd build-mcu
cmake -S .. -B . \
  -DCMAKE_BUILD_TYPE=Release \
  -DCCSDSPACK_BUILD_MCU=ON \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/arm-none-eabi.cmake \
  -DMCU_FLAGS="-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
cmake --build . -j
```

---
## package.sh reference

`package.sh` is a convenience wrapper around CMake and CPack. It accepts the following options:

- `-p, --package-type`  Package type: `DEB`, `RPM`, `TGZ`, `MCU` (default: `DEB`).
- `-t, --toolchain`     Path to a CMake toolchain file for cross-builds (e.g., `cmake/toolchains/aarch64-linux-gnu.cmake`).
- `-m, --mcu-flags`     Extra flags to use when `-p MCU` is selected (e.g., Cortex‑M tuning flags).
- `-h, --help`          Show usage and examples.

Examples:

```bash
# Native x86_64 .deb
./package.sh -p DEB

# aarch64 .deb
./package.sh -p DEB -t cmake/toolchains/aarch64-linux-gnu.cmake

# Bare‑metal Cortex‑M7 archive (TGZ)
./package.sh -p MCU -t cmake/toolchains/arm-none-eabi.cmake \
  -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
```

Artifacts are written to `./packages/`.

---
## Troubleshooting

- `E: Unable to locate package libc6:arm64` on 20.04
  - Follow the Focal-specific dual-arch setup using `ports.ubuntu.com` above.
- `cpack` produces no artifacts
  - Ensure the project built successfully and that CPack configuration is enabled for the chosen generator.
- aarch64 binary can’t run locally
  - Use `qemu-user-static` or test on an actual arm64 machine.
- Building in CI
  - See `.github/workflows/linux.yml` — on ubuntu-22.04 runners we only add `arm64` and install the required `:arm64` libs without rewriting sources.
