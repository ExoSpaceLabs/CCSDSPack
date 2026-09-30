# Notice

CCSDSPack does not vendor STM32CubeH7, STM32 HAL/LL, STM32CubeIDE projects, or
Device Abstraction Stack source code.

The NUCLEO-H755ZI-Q hardware-validation workflow uses pinned external source
checkouts at build time:

- `Inczert/device-abstraction-stack` for platform/startup/UART support;
- STMicroelectronics STM32CubeH7 and `cmsis-device-h7` for CMSIS headers only.

Those external components remain governed by their own upstream licenses and
are not redistributed as source files in this repository.
