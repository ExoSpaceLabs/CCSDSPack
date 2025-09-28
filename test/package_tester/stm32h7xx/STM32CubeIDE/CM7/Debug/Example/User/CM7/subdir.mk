################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM7/Src/main.cpp 

C_SRCS += \
/home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM7/Src/stm32h7xx_hal_msp.c \
/home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM7/Src/stm32h7xx_it.c \
../Example/User/CM7/syscalls.c \
../Example/User/CM7/sysmem.c 

C_DEPS += \
./Example/User/CM7/stm32h7xx_hal_msp.d \
./Example/User/CM7/stm32h7xx_it.d \
./Example/User/CM7/syscalls.d \
./Example/User/CM7/sysmem.d 

OBJS += \
./Example/User/CM7/main.o \
./Example/User/CM7/stm32h7xx_hal_msp.o \
./Example/User/CM7/stm32h7xx_it.o \
./Example/User/CM7/syscalls.o \
./Example/User/CM7/sysmem.o 

CPP_DEPS += \
./Example/User/CM7/main.d 


# Each subdirectory must supply rules for building sources it contributes
Example/User/CM7/main.o: /home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM7/Src/main.cpp Example/User/CM7/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++17 -g -DDEBUG -DUSE_HAL_DRIVER -DUSE_PWR_DIRECT_SMPS_SUPPLY -DCORE_CM7 -DUSE_STM32H7XX_NUCLEO_144_MB1363 -DSTM32H745xx -c -I../../../Common/Inc -I../../../CM7/Inc -I../../../../../../../../Drivers/CMSIS/Include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../../../../../../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../../../../../../../Drivers/BSP/STM32H7xx_Nucleo -I../../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../../Utilities/Fonts -I../../../../../../../../Utilities/CPU -I/home/dev/Middlewares/Third_Party/CCSDSPack/includes -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Example/User/CM7/stm32h7xx_hal_msp.o: /home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM7/Src/stm32h7xx_hal_msp.c Example/User/CM7/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DUSE_PWR_DIRECT_SMPS_SUPPLY -DCORE_CM7 -DUSE_STM32H7XX_NUCLEO_144_MB1363 -DSTM32H745xx -c -I../../../Common/Inc -I../../../CM7/Inc -I../../../../../../../../Drivers/CMSIS/Include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../../../../../../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../../../../../../../Drivers/BSP/STM32H7xx_Nucleo -I../../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../../Utilities/Fonts -I../../../../../../../../Utilities/CPU -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Example/User/CM7/stm32h7xx_it.o: /home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM7/Src/stm32h7xx_it.c Example/User/CM7/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DUSE_PWR_DIRECT_SMPS_SUPPLY -DCORE_CM7 -DUSE_STM32H7XX_NUCLEO_144_MB1363 -DSTM32H745xx -c -I../../../Common/Inc -I../../../CM7/Inc -I../../../../../../../../Drivers/CMSIS/Include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../../../../../../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../../../../../../../Drivers/BSP/STM32H7xx_Nucleo -I../../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../../Utilities/Fonts -I../../../../../../../../Utilities/CPU -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Example/User/CM7/%.o Example/User/CM7/%.su Example/User/CM7/%.cyclo: ../Example/User/CM7/%.c Example/User/CM7/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DUSE_PWR_DIRECT_SMPS_SUPPLY -DCORE_CM7 -DUSE_STM32H7XX_NUCLEO_144_MB1363 -DSTM32H745xx -c -I../../../Common/Inc -I../../../CM7/Inc -I../../../../../../../../Drivers/CMSIS/Include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../../../../../../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../../../../../../../Drivers/BSP/STM32H7xx_Nucleo -I../../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../../Utilities/Fonts -I../../../../../../../../Utilities/CPU -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Example-2f-User-2f-CM7

clean-Example-2f-User-2f-CM7:
	-$(RM) ./Example/User/CM7/main.cyclo ./Example/User/CM7/main.d ./Example/User/CM7/main.o ./Example/User/CM7/main.su ./Example/User/CM7/stm32h7xx_hal_msp.cyclo ./Example/User/CM7/stm32h7xx_hal_msp.d ./Example/User/CM7/stm32h7xx_hal_msp.o ./Example/User/CM7/stm32h7xx_hal_msp.su ./Example/User/CM7/stm32h7xx_it.cyclo ./Example/User/CM7/stm32h7xx_it.d ./Example/User/CM7/stm32h7xx_it.o ./Example/User/CM7/stm32h7xx_it.su ./Example/User/CM7/syscalls.cyclo ./Example/User/CM7/syscalls.d ./Example/User/CM7/syscalls.o ./Example/User/CM7/syscalls.su ./Example/User/CM7/sysmem.cyclo ./Example/User/CM7/sysmem.d ./Example/User/CM7/sysmem.o ./Example/User/CM7/sysmem.su

.PHONY: clean-Example-2f-User-2f-CM7

