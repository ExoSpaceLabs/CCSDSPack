# Packages

[Documentation index](README.md) | [Cross-build guide](CROSSBUILD.md) | [v1.2 hardware validation](V1_2_HARDWARE_VALIDATION.md)

## Linux packages

`package.sh` configures the project, builds it, and invokes CPack. Run the build as a normal user:

```bash
./package.sh -p DEB
```

Supported options:

- `-p` or `--package-type`: package type, one of `DEB`, `RPM`, `TGZ`, or `MCU`; default is `DEB`.
- `-t` or `--toolchain`: optional CMake toolchain file for cross-builds.
- `-m` or `--mcu-flags`: additional MCU compiler flags when producing an `MCU` package.
- `--help`: show command usage and examples.

Successful packages are written under `packages/`.

Do not build with `sudo`. Root privileges are only required when installing or removing a system package. Building as root creates root-owned repository files and contributes nothing except future irritation.

### RPM prerequisite

```bash
sudo apt-get update
sudo apt-get install -y rpm
rpmbuild --version
```

## Installing a DEB

System package installation requires root permissions:

```bash
sudo dpkg -i packages/ccsdspack-v<version>-Linux-<architecture>.deb
```

Inspect the installed package:

```bash
dpkg -s ccsdspack
dpkg -L ccsdspack
```

Remove it with:

```bash
sudo dpkg --remove ccsdspack
```

## Installed test fixtures

Packages that include `CCSDSPack_tester` also install a sibling `test_resources` directory under the executable installation directory. The tester uses relative `test_resources/...` paths for committed fixtures and temporary file-I/O round trips.

These resources belong only to the regression tester. They are not a dependency of:

- `libccsdspack`;
- `ccsds_encoder`;
- `ccsds_decoder`;
- `ccsds_validator`;
- applications linking `ccsdspack::CCSDSPack`.

Applications must not treat the installed test resources as public runtime data or API assets.

## CMake package consumption

After installation, use the exported CMake package:

```cmake
find_package(CCSDSPack CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE ccsdspack::CCSDSPack)
```

A release consumer can require the exact version:

```cmake
find_package(CCSDSPack 1.2.0 EXACT CONFIG REQUIRED)
```

## Raspberry Pi arm64 validation

On a native 64-bit Raspberry Pi system, run the complete installed-package validation from a checkout matching the package candidate:

```bash
ARM64_DEB="$(find ./packages -type f -name '*arm64*.deb' -print -quit)"

bash test/package_tester/aarch64_validate.sh "$ARM64_DEB" \
  2>&1 | tee ~/ccsdspack-aarch64-validation.log
```

Launch the script as a normal user. It invokes `sudo dpkg -i` because package installation requires root permissions. It then copies the installed test-only fixtures into a writable temporary directory, preserves their expected relative layout, and runs the tester unprivileged without writing into `/bin`.

The required final marker is:

```text
CCSDSPACK_AARCH64_TEST:PASS
```

The recorded v1.2 Raspberry Pi 5 result and complete reproduction procedure are in [v1.2 hardware validation](V1_2_HARDWARE_VALIDATION.md).

## Cross-builds and bare metal

For aarch64 Linux cross-compilation and bare-metal Cortex-M packaging, see the [Cross-build guide](CROSSBUILD.md). It documents toolchain prerequisites and `package.sh` examples.

The STM32H745 reference harness and STM32H755-native integration guidance are under:

```text
test/package_tester/stm32h7xx/
```
