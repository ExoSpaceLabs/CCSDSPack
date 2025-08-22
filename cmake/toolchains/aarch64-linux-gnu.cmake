# cmake/toolchains/aarch64-linux-gnu.cmake
# Cross-build for Raspberry Pi 5 (arm64 / aarch64)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Compilers
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_AR           aarch64-linux-gnu-ar)
set(CMAKE_RANLIB       aarch64-linux-gnu-ranlib)
set(CMAKE_STRIP        aarch64-linux-gnu-strip)

# Optional sysroot; pass -DRPI_SYSROOT=/path/to/sysroot
if(DEFINED RPI_SYSROOT AND EXISTS "${RPI_SYSROOT}")
    set(CMAKE_SYSROOT "${RPI_SYSROOT}")
    # Helps CMake find headers/libs in the sysroot
    set(CMAKE_FIND_ROOT_PATH "${RPI_SYSROOT}")
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

# Optimize a bit; tune to your taste
add_compile_options(-O2 -fPIC)
