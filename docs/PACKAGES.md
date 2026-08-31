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
find_package(CCSDSPack 2.0 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE ccsdspack::CCSDSPack)
target_compile_features(my_app PRIVATE cxx_std_17)
```

Consumers that require the exact release can use:

```cmake
find_package(CCSDSPack 2.0.0 EXACT CONFIG REQUIRED)
```

The installed package exports the C++17 library API, including Packet, Manager, PUS codecs/tailoring, CUC time, raw-buffer adapters, Result/Error, and structured Validator.

## Linux packages

Example DEB installation:

```bash
sudo dpkg -i packages/ccsdspack-v<version>-Linux-<architecture>.deb
```

Packages that include `CCSDSPack_tester` also install its `test_resources` fixtures. These fixtures are regression-test inputs and are not runtime dependencies of the library or CLI tools.

## arm64 validation

A native 64-bit Raspberry Pi or equivalent arm64 target can validate an installed DEB with:

```bash
ARM64_DEB="$(find ./packages -type f -name '*arm64*.deb' -print -quit)"
bash test/package_tester/aarch64_validate.sh "$ARM64_DEB" \
  2>&1 | tee ~/ccsdspack-aarch64-validation.log
```

Successful release evidence ends with:

```text
CCSDSPACK_AARCH64_TEST:PASS
```

## Bare-metal package

The MCU path uses `CCSDSPACK_BUILD_MCU=ON`, C++17, and optional `CCSDSPACK_MCU_FLAGS`. It contains the protocol library and excludes hosted configuration/CLI components.

The STM32H7 reference harness is under `test/package_tester/stm32h7xx/`. Physical execution is recorded separately from cross-build/compile-link evidence.

See [CROSSBUILD.md](CROSSBUILD.md).
