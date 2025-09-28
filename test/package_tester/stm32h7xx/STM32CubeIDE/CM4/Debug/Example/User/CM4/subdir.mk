################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM4/Src/main.c \
/home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM4/Src/stm32h7xx_hal_msp.c \
/home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM4/Src/stm32h7xx_it.c \
../Example/User/CM4/syscalls.c \
../Example/User/CM4/sysmem.c 

C_DEPS += \
./Example/User/CM4/main.d \
./Example/User/CM4/stm32h7xx_hal_msp.d \
./Example/User/CM4/stm32h7xx_it.d \
./Example/User/CM4/syscalls.d \
./Example/User/CM4/sysmem.d 

OBJS += \
./Example/User/CM4/main.o \
./Example/User/CM4/stm32h7xx_hal_msp.o \
./Example/User/CM4/stm32h7xx_it.o \
./Example/User/CM4/syscalls.o \
./Example/User/CM4/sysmem.o 


# Each subdirectory must supply rules for building sources it contributes
Example/User/CM4/main.o: /home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM4/Src/main.c Example/User/CM4/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32H745xx -DCORE_CM4 -DUSE_HAL_DRIVER -DUSE_PWR_DIRECT_SMPS_SUPPLY -DUSE_STM32H7XX_NUCLEO_144_SMPS -c -I../../../Common/Inc -I../../../CM4/Inc -I../../../../../../../../Drivers/CMSIS/Include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../../../../../../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../../../../../../../Drivers/BSP/STM32H7xx_Nucleo -I../../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../../Utilities/Fonts -I../../../../../../../../Utilities/CPU -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Example/User/CM4/stm32h7xx_hal_msp.o: /home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM4/Src/stm32h7xx_hal_msp.c Example/User/CM4/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32H745xx -DCORE_CM4 -DUSE_HAL_DRIVER -DUSE_PWR_DIRECT_SMPS_SUPPLY -DUSE_STM32H7XX_NUCLEO_144_SMPS -c -I../../../Common/Inc -I../../../CM4/Inc -I../../../../../../../../Drivers/CMSIS/Include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../../../../../../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../../../../../../../Drivers/BSP/STM32H7xx_Nucleo -I../../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../../Utilities/Fonts -I../../../../../../../../Utilities/CPU -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Example/User/CM4/stm32h7xx_it.o: /home/dev/Works/CCSDSPack/test/package_tester/stm32h7xx/CM4/Src/stm32h7xx_it.c Example/User/CM4/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32H745xx -DCORE_CM4 -DUSE_HAL_DRIVER -DUSE_PWR_DIRECT_SMPS_SUPPLY -DUSE_STM32H7XX_NUCLEO_144_SMPS -c -I../../../Common/Inc -I../../../CM4/Inc -I../../../../../../../../Drivers/CMSIS/Include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../../../../../../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../../../../../../../Drivers/BSP/STM32H7xx_Nucleo -I../../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../../Utilities/Fonts -I../../../../../../../../Utilities/CPU -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Example/User/CM4/%.o Example/User/CM4/%.su Example/User/CM4/%.cyclo: ../Example/User/CM4/%.c Example/User/CM4/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32H745xx -DCORE_CM4 -DUSE_HAL_DRIVER -DUSE_PWR_DIRECT_SMPS_SUPPLY -DUSE_STM32H7XX_NUCLEO_144_SMPS -c -I../../../Common/Inc -I../../../CM4/Inc -I../../../../../../../../Drivers/CMSIS/Include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../../../../../../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../../../../../../../Drivers/BSP/STM32H7xx_Nucleo -I../../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../../Utilities/Fonts -I../../../../../../../../Utilities/CPU -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Example-2f-User-2f-CM4

clean-Example-2f-User-2f-CM4:
	-$(RM) ./Example/User/CM4/main.cyclo ./Example/User/CM4/main.d ./Example/User/CM4/main.o ./Example/User/CM4/main.su ./Example/User/CM4/stm32h7xx_hal_msp.cyclo ./Example/User/CM4/stm32h7xx_hal_msp.d ./Example/User/CM4/stm32h7xx_hal_msp.o ./Example/User/CM4/stm32h7xx_hal_msp.su ./Example/User/CM4/stm32h7xx_it.cyclo ./Example/User/CM4/stm32h7xx_it.d ./Example/User/CM4/stm32h7xx_it.o ./Example/User/CM4/stm32h7xx_it.su ./Example/User/CM4/syscalls.cyclo ./Example/User/CM4/syscalls.d ./Example/User/CM4/syscalls.o ./Example/User/CM4/syscalls.su ./Example/User/CM4/sysmem.cyclo ./Example/User/CM4/sysmem.d ./Example/User/CM4/sysmem.o ./Example/User/CM4/sysmem.su

.PHONY: clean-Example-2f-User-2f-CM4

