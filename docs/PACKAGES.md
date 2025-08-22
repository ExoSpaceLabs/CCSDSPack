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
Two parameters can be passed to the package.sh script:
- `-p` or `--package-type` : to specify type of package to build `[DEB, RPM, TGZ]`, default `DEB`.
- `-t` or `--toolchain` : optionally to specify toolchain file for cross builds, default is none i.e. use system.

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

### Cross-compile
The toolchain is provided in `cmake/toolchains` to build for arm64 / aarch64

```bash

sudo apt update
sudo apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
                    qemu-user-static rsync libc6:arm64 libgcc-s1:arm64 \
                    libstdc++6:arm64
        
```
If your system is not able to find the arm64 libraries enable dpkg by running

```bash        
# 1) add arm64 architecture
sudo dpkg --add-architecture arm64
dpkg --print-foreign-architectures   # should show: arm64 (and i386 if you added it)

# 2) Backup current sources
sudo cp /etc/apt/sources.list /etc/apt/sources.list.bak.$(date +%s)

# 3) Write a clean dual-arch sources.list
sudo tee /etc/apt/sources.list >/dev/null <<'EOF'
# amd64 from main archive
deb [arch=amd64] http://archive.ubuntu.com/ubuntu jammy main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu jammy-updates main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu jammy-backports main restricted universe multiverse
deb [arch=amd64] http://security.ubuntu.com/ubuntu jammy-security main restricted universe multiverse

# arm64 from ports (the place that actually serves non-amd64 reliably)
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-updates main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-backports main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-security main restricted universe multiverse
EOF

# 4) Clean and refresh indexes (force IPv4 because some networks are drama)
sudo apt-get clean
sudo rm -rf /var/lib/apt/lists/*
sudo apt-get -o Acquire::ForceIPv4=true update

# 5) Install the cross toolchain and the arm64 runtimes shlibdeps wants
sudo apt-get install -y \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static rsync \
  libc6:arm64 libgcc-s1:arm64 libstdc++6:arm64

# 6) Sanity check that the files exist
dpkg -L libc6:arm64        | grep -E 'ld-linux-aarch64\.so\.1|libc\.so\.6' || true
dpkg -L libgcc-s1:arm64    | grep libgcc_s.so.1 || true
dpkg -L libstdc++6:arm64   | grep 'libstdc\+\+\.so\.6' || true
```

### Bare-metal
For bare-metal cross-compilation and static lib please refer to [cross-cpmpile](CROSSCOMPILE.md).

