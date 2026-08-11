# Packages

[Documentation index](README.md) | [Cross-build guide](CROSSBUILD.md) | [v1.2 hardware validation](V1_2_HARDWARE_VALIDATION.md)

## Linux packages

`package.sh` configures the project, builds it, and invokes CPack. Run the build as a normal user:

```bash
./package.sh -p DEB
```

Supported options:

- `-p` or `--package-type`: `DEB`, `RPM`, `TGZ`, or `MCU`; default is `DEB`;
- `-t` or `--toolchain`: optional CMake toolchain file for cross-builds;
- `-m` or `--mcu-flags`: additional MCU compiler flags, forwarded to the MCU library build and compile/link probe;
- `--help`: command usage and examples.

Successful packages are written under `packages/`.

Do not build with `sudo`. Root privileges are only required when installing or removing a system package.

### RPM prerequisite

```bash
sudo apt-get update
sudo apt-get install -y rpm
rpmbuild --version
```

## Installing a DEB

```bash
sudo dpkg -i packages/ccsdspack-v<version>-Linux-<architecture>.deb
```

Inspect or remove it with:

```bash
dpkg -s ccsdspack
dpkg -L ccsdspack
sudo dpkg --remove ccsdspack
```

## Installed test fixtures

Packages that include `CCSDSPack_tester` also install a sibling `test_resources`
directory under the executable installation directory. These fixtures are for
the regression suite only. They are not runtime dependencies of the library,
the CLI programs, or external applications.

## CMake package consumption

After installation:

```cmake
find_package(CCSDSPack CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE ccsdspack::CCSDSPack)
```

A v2 consumer can require the breaking-release API explicitly:

```cmake
find_package(CCSDSPack 2.0.0 EXACT CONFIG REQUIRED)
```

The package exports the C++17 library API, including the structured Validator.

## Raspberry Pi arm64 validation

On a native 64-bit Raspberry Pi system, run the installed-package validation
from a checkout matching the package candidate:

```bash
ARM64_DEB="$(find ./packages -type f -name '*arm64*.deb' -print -quit)"

bash test/package_tester/aarch64_validate.sh "$ARM64_DEB" \
  2>&1 | tee ~/ccsdspack-aarch64-validation.log
```

The required final marker is:

```text
CCSDSPACK_AARCH64_TEST:PASS
```

The recorded v1.2 Raspberry Pi 5 result remains historical regression evidence.
The v2 release records a fresh arm64 result under the v2 release-validation gate.

## Cross-builds and bare metal

For aarch64 Linux cross-compilation and the bare-metal Cortex-M static library,
see [CROSSBUILD.md](CROSSBUILD.md).

The MCU package path uses `CCSDSPACK_BUILD_MCU=ON`, C++17, and the supplied
`CCSDSPACK_MCU_FLAGS`. A typical Cortex-M7 build disables exceptions and RTTI.
The MCU package contains the protocol library, including `ccsds::Validator`; it
does not contain host-only CLI/configuration components.

The STM32H7 reference harness is under:

```text
test/package_tester/stm32h7xx/
```

Historical v1.2 STM32 evidence is not treated as the final v2 hardware result;
representative v2 PUS/Validator execution is recorded separately before release.
