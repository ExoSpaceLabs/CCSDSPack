################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/dev/Drivers/BSP/STM32H7xx_Nucleo/stm32h7xx_nucleo.c 

C_DEPS += \
./Drivers/BSP/STM32H745I_Nucleo/stm32h7xx_nucleo.d 

OBJS += \
./Drivers/BSP/STM32H745I_Nucleo/stm32h7xx_nucleo.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/STM32H745I_Nucleo/stm32h7xx_nucleo.o: /home/dev/Drivers/BSP/STM32H7xx_Nucleo/stm32h7xx_nucleo.c Drivers/BSP/STM32H745I_Nucleo/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32H745xx -DCORE_CM4 -DUSE_HAL_DRIVER -DUSE_PWR_DIRECT_SMPS_SUPPLY -DUSE_STM32H7XX_NUCLEO_144_SMPS -c -I../../../Common/Inc -I../../../CM4/Inc -I../../../../../../../../Drivers/CMSIS/Include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../../../../../../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../../../../../../../Drivers/BSP/STM32H7xx_Nucleo -I../../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../../Utilities/Fonts -I../../../../../../../../Utilities/CPU -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-STM32H745I_Nucleo

clean-Drivers-2f-BSP-2f-STM32H745I_Nucleo:
	-$(RM) ./Drivers/BSP/STM32H745I_Nucleo/stm32h7xx_nucleo.cyclo ./Drivers/BSP/STM32H745I_Nucleo/stm32h7xx_nucleo.d ./Drivers/BSP/STM32H745I_Nucleo/stm32h7xx_nucleo.o ./Drivers/BSP/STM32H745I_Nucleo/stm32h7xx_nucleo.su

.PHONY: clean-Drivers-2f-BSP-2f-STM32H745I_Nucleo

