# Packages

[../](README.md) - CCSDSPack Documentation

## Linux
A sample bash script has been prepared to generate .deb package using cpack.

The script can be launched using the following command, note for generation
super/user privileges might be required. In that case use `sudo` command 
before running `bash`.

```bash

bash package.sh
```
Parameters can be passed to the package.sh script:
- `-p` or `--package-type` : to specify type of package to build `[DEB, RPM, TGZ, MCU]`, default `DEB`.
- `-t` or `--toolchain` : optionally to specify toolchain file for cross builds, default is none i.e. use system.
- `-m` or `--mcu-flags` : optionally and if MCU type is used, custom flags can be provided. 
- `--help` : prints the help menu and examples.

***NOTE:*** Dependencies for cross-builds are to be resolved manually see [aarch](#cross-compile)

If successful, the generated package will be placed under the `packages` directory.

to build RPM packages, please make sure that rpm is installed.

```bash

sudo apt-get update
sudo apt-get install -y rpm
# sanity check
rpmbuild --version

```

On debian system, the generated package can be installed using the following command:
```bash

dpkg -i ccsdspack-v{version}-{system}-{architecture}.deb
```

and uninstalled using `dpkg`
```bash

dpkg --remove ccsdspack
```
Once installed, cmake `find_package()` can be used to find the library and link it as follows:
```cmake

find_package(CCSDSPack CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE ccsdspack::CCSDSPack)
```

### Cross-build
For cross-compilation to aarch64/arm64 (Linux) and for MCU targets, please refer to the consolidated guide:

- [Cross-Build Guide (aarch64 Linux and Bare-metal MCU)](CROSSBUILD.md)

It contains per-Ubuntu release prerequisites (22.04 vs 20.04), toolchain setup, and `package.sh` examples.

### Bare-metal
For bare-metal cross-compilation and static library, see the [Cross-Build Guide](CROSSBUILD.md).

