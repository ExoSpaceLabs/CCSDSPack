# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0

# cmake/toolchains/arm-none-eabi.cmake
# Generic bare-metal ARM toolchain for CCSDSPack MCU builds

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_NONE_EABI_GCC arm-none-eabi-gcc)
find_program(ARM_NONE_EABI_GPP arm-none-eabi-g++)
find_program(ARM_NONE_EABI_AR  arm-none-eabi-ar)
find_program(ARM_NONE_EABI_RANLIB arm-none-eabi-ranlib)

if(NOT ARM_NONE_EABI_GCC OR NOT ARM_NONE_EABI_GPP)
  message(FATAL_ERROR "arm-none-eabi-gcc/g++ not found. Install: gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib")
endif()

set(CMAKE_C_COMPILER   ${ARM_NONE_EABI_GCC})
set(CMAKE_CXX_COMPILER ${ARM_NONE_EABI_GPP})
set(CMAKE_ASM_COMPILER ${ARM_NONE_EABI_GCC})
set(CMAKE_AR           ${ARM_NONE_EABI_AR})
set(CMAKE_RANLIB       ${ARM_NONE_EABI_RANLIB})

# Defaults; override with -DMCU_FLAGS="...".
#
# MCU_FLAGS is kept as the compatibility input used by existing package/build
# commands. Some historically supplied options are C++-only, so derive a
# language-neutral variant before feeding the C/ASM compiler and final linker.
if(NOT DEFINED MCU_FLAGS)
  set(MCU_FLAGS "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
endif()

set(MCU_COMMON_FLAGS "${MCU_FLAGS}")
foreach(_cxx_only_flag
    "-fno-exceptions"
    "-fexceptions"
    "-fno-rtti"
    "-frtti"
    "-fno-threadsafe-statics")
  string(REPLACE "${_cxx_only_flag}" "" MCU_COMMON_FLAGS "${MCU_COMMON_FLAGS}")
endforeach()
string(REGEX REPLACE "[ \t]+" " " MCU_COMMON_FLAGS "${MCU_COMMON_FLAGS}")
string(STRIP "${MCU_COMMON_FLAGS}" MCU_COMMON_FLAGS)

set(CMAKE_C_FLAGS_INIT   "${MCU_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${MCU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${MCU_COMMON_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${MCU_COMMON_FLAGS}")
