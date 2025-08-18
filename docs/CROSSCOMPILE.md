# Cross-Compilation

## Baremetal Static Library

The library can be built in MCU mode by using the cmake flag `-DCCSDSPACK_BUILD_MCU=ON`, this disables all exceptions
and executables. If this version is required by cmake use `find_packa()` as follows:

```cmake
find_package(CCSDSPack REQUIRED)
target_link_libraries(app PRIVATE ccsdspack::mcu)      # static MCU
```


## Requirements
**NOTE:*** This example is performed targeting an MCU with ARM processor.  

To perform a successful build required building tools are to be installed on the system:

```bash

sudo apt update
sudo apt install -y \
gcc-arm-none-eabi \
binutils-arm-none-eabi \
libnewlib-arm-none-eabi \
libstdc++-arm-none-eabi-newlib
```
# Build

Use the following commands while passing the correct toolchain to build the static library:  
```bash

mkdir build-mcu && cd build-mcu
cmake .. \
-DCCSDSPACK_BUILD_MCU=ON \
-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
-DCCSDSPACK_MCU_FLAGS="-fno-exceptions;-fno-rtti"
cmake --build . -j
```