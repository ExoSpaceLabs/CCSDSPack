# Packages

[Documentation index](README.md) | [Cross-build guide](CROSSBUILD.md)

## Package generation

`package.sh` configures, builds, and invokes CPack:

```bash
./package.sh -p DEB
```

Supported options:

- `-p` / `--package-type`: `DEB`, `RPM`, `TGZ`, or `MCU`;
- `-t` / `--toolchain`: optional CMake toolchain file;
- `-m` / `--mcu-flags`: additional MCU compiler flags forwarded to the library and compile/link probe;
- `--help`: usage.

Artifacts are written under `packages/`. Package generation should run as a normal user; elevated privileges are only needed for system installation/removal.

## Installed CMake package

```cmake
find_package(CCSDSPack 2.1 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE ccsdspack::CCSDSPack)
target_compile_features(my_app PRIVATE cxx_std_17)
```

Consumers that require the exact release can use:

```cmake
find_package(CCSDSPack 2.1.0 EXACT CONFIG REQUIRED)
```

The default hosted package exports both `ccsdspack::c` (the C11 protocol core) and `ccsdspack::CCSDSPack` (the C++17 ownership/convenience layer). A build configured with `CCSDSPACK_BUILD_CPP=OFF` exports **only** `ccsdspack::c` and installs only the standalone C headers. The MCU build exports the static core and, when enabled, the C++17 archive; hosted configuration, file-I/O, and CLI helpers are not MCU library APIs. The internal `exec_utils.h` CLI helper is not part of installed library headers.

## Linux packages

Example DEB installation:

```bash
sudo dpkg -i packages/ccsdspack-v<version>-Linux-<architecture>.deb
```

Packages that include `CCSDSPack_tester` also install its `test_resources` fixtures. These fixtures are regression-test inputs and are not runtime dependencies of the library or CLI tools.

## arm64 validation

A native 64-bit Raspberry Pi or equivalent arm64 target can build and validate the current source tree in one command:

```bash
bash test/package_tester/run_aarch64_validation.sh
```

The lower-level `aarch64_validate.sh <package.deb>` entry point remains available when an already-built package must be qualified directly.

Successful full-run evidence ends with:

```text
CCSDSPACK_HARDWARE_TEST:PASS
CCSDSPACK_AARCH64_TEST:PASS
CCSDSPACK_AARCH64_RUNNER:PASS
```

## Bare-metal package

The MCU path uses `CCSDSPACK_BUILD_MCU=ON` and optional `CCSDSPACK_MCU_FLAGS`. It builds the C11 core and, when `CCSDSPACK_BUILD_CPP=ON`, the compatible C++17 static library while excluding hosted configuration/CLI components.

The STM32H7 reference harness is under `test/package_tester/stm32h7xx/`. Physical execution is recorded separately from cross-build/compile-link evidence.

See [CROSSBUILD.md](CROSSBUILD.md).
