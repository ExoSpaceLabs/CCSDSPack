cmake_minimum_required(VERSION 3.20)

set(DAS_CORE "cm7" CACHE STRING "DAS core" FORCE)
project(ccsdspack_stm32h755_validation LANGUAGES C CXX ASM)

include(FetchContent)

set(CCSDSPACK_DAS_REF
    "bf0af2a0c53d361ed8bb271b4160dd3967b28af8"
    CACHE STRING "Pinned device-abstraction-stack commit")
set(CCSDSPACK_VALIDATION_SOURCE_SHA "unknown" CACHE STRING "CCSDSPack source SHA represented by the package")
set(CCSDSPACK_VALIDATION_PACKAGE_SHA256 "unknown" CACHE STRING "MCU package SHA-256")
set(CCSDSPACK_VALIDATION_LIBRARY_SHA256 "unknown" CACHE STRING "libccsdspack.a SHA-256")

if(NOT STM32_CUBE_H7_DIR)
    message(FATAL_ERROR "STM32_CUBE_H7_DIR must point at the CMSIS-only STM32CubeH7 layout")
endif()

find_package(CCSDSPack 2.1 CONFIG REQUIRED)

set(DAS_DEVICE "nucleo_h755zi_q" CACHE STRING "" FORCE)
set(DAS_CORE "cm7" CACHE STRING "" FORCE)
set(DAS_BUILD_HARDWARE_TESTS OFF CACHE BOOL "" FORCE)
set(DAS_BUILD_LINK_TESTS OFF CACHE BOOL "" FORCE)
set(DAS_USE_DEFAULT_VECTOR_TABLE ON CACHE BOOL "" FORCE)
set(STM32_CUBE_H7_DIR "${STM32_CUBE_H7_DIR}" CACHE PATH "" FORCE)

FetchContent_Declare(
    das
    GIT_REPOSITORY https://github.com/Inczert/device-abstraction-stack.git
    GIT_TAG ${CCSDSPACK_DAS_REF}
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(das)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/validation_build_info.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/validation_build_info.h"
    @ONLY
)

add_executable(ccsdspack_stm32h755_validation main.cpp)
target_link_libraries(ccsdspack_stm32h755_validation
    PRIVATE
        ccsdspack::CCSDSPack
        das::das
)
target_include_directories(ccsdspack_stm32h755_validation
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../hardware"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_compile_features(ccsdspack_stm32h755_validation PRIVATE cxx_std_17)
target_compile_options(ccsdspack_stm32h755_validation PRIVATE
    -Wall
    -Wextra
    -Werror
    -ffreestanding
    -fno-builtin
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -ffunction-sections
    -fdata-sections
)
target_link_options(ccsdspack_stm32h755_validation PRIVATE
    -nostartfiles
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--unresolved-symbols=report-all
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/ccsdspack_stm32h755_validation.map
)
set_target_properties(ccsdspack_stm32h755_validation PROPERTIES SUFFIX ".elf")

find_program(ARM_NONE_EABI_SIZE arm-none-eabi-size)
if(ARM_NONE_EABI_SIZE)
    add_custom_command(TARGET ccsdspack_stm32h755_validation POST_BUILD
        COMMAND ${ARM_NONE_EABI_SIZE} $<TARGET_FILE:ccsdspack_stm32h755_validation>
        VERBATIM
    )
endif()
