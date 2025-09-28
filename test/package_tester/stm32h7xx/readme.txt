/**
  @page UART_WakeUpFromStopUsingFIFO  wake up from STOP mode using UART FIFO level example
  
  @verbatim
  ******************************************************************************
  * @file    UART/UART_WakeUpFromStopUsingFIFO/readme.txt 
  * @author  MCD Application Team
  * @brief   Description of the wake up from STOP mode using UART FIFO level example.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @endverbatim

@par Example Description

This example shows how to use UART HAL API to wake up the MCU from STOP mode
using the UART FIFO level.

This CCSDS library test has been extendeed from the Examples/UART example project
provided by STMicroelectronics git repository link beloew:
https://github.com/STMicroelectronics/STM32CubeH7

Follow instructions install STM32CubeIDE and Programmer. 
NOTE: Make sure all paths are as expected. CCSDSPack is expected to be under middleware.
The library is built with expected flags.
Example: Build static library with the following command:

./package.sh -t cmake/toolchains/arm-none-eabi.cmake -p MCU -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"

Extract the generated package under Middleware/3rdParty

Fix paths and build stm32 project. Program the device and test.

connect to device using ST-Link via

picocom -b 9600 -d 7 -p o -f n /dev/ttyACM0

Once this has been achieved any button can be hit to start CCSDS pack auto tester.

Producer notes continued:

Board: NUCLEO-H745ZI-Q
Tx Pin: PD.8
Rx Pin: PD.9
   _________________________
  |           ______________|                       _______________
  |          |USART         |                      | HyperTerminal |
  |          |              |                      |               |
  |          |           TX |______________________|RX             |
  |          |              |                      |               |
  |          |              |     ST-Link Cable    |               |
  |          |              |                      |               |
  |          |           RX |______________________|TX             |
  |          |              |                      |               |
  |          |______________|                      |_______________|
  |                         |
  |                         |
  |                         |
  |                         |
  |_STM32_Board_____________|

LED1 is ON when MCU is not in STOP mode.
LED3 is ON when there is an error occurrence.

The UART is configured as follows:
    - BaudRate = 9600 baud  
    - Word Length = 8 Bits (7 data bit + 1 parity bit)
    - One Stop Bit
    - Odd parity
    - Hardware flow control disabled (RTS and CTS signals)
    - Reception and transmission are enabled in the time

@note USARTx/UARTx instance used and associated resources can be updated in "main.h"
      file depending hardware configuration used.

@note When the parity is enabled, the computed parity is inserted at the MSB
      position of the transmitted data.

@note Care must be taken when using HAL_Delay(), this function provides accurate delay (in milliseconds)
      based on variable incremented in SysTick ISR. This implies that if HAL_Delay() is called from
      a peripheral ISR process, then the SysTick interrupt must have higher priority (numerically lower)
      than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
      To change the SysTick interrupt priority you have to use HAL_NVIC_SetPriority() function.
      
@note The application needs to ensure that the SysTick time base is always set to 1 millisecond
      to have correct HAL operation.

@Note If the  application is using the DTCM/ITCM memories (@0x20000000/ 0x0000000: not cacheable and only accessible
      by the Cortex M7 and the �MDMA), no need for cache maintenance when the Cortex M7 and the MDMA access these RAMs.
����� If the application needs to use DMA(or other masters) based access or requires more RAM, then �the user has to:
����������� � - Use a non TCM SRAM. (example : D1 AXI-SRAM @ 0x24000000)
����������� � - Add a cache maintenance mechanism to ensure the cache coherence between CPU and other masters(DMAs,DMA2D,LTDC,MDMA).
�������       - The addresses and the size of cacheable buffers (shared between CPU and other masters)
                must be	properly�defined to be aligned to L1-CACHE line size (32 bytes). 
�
@Note It is recommended to enable the cache and maintain its coherence.
      Depending on the use case it is also possible to configure the cache attributes using the MPU.
������Please refer to the AN4838 "Managing memory protection unit (MPU) in STM32 MCUs"
������Please refer to the AN4839 "Level 1 cache on STM32F7 Series"

@par Keywords

Connectivity, UART, Baud rate, RS-232, Full-duplex, Parity, Stop bit, Transmission, Reception, FIFO, Threshold, Interruption, STOP, Low power, Wakeup, HyperTerminal

@par Directory contents 

  - UART/UART_WakeUpFromStopUsingFIFO/Inc/stm32h7xx_hal_conf.h    HAL configuration file
  - UART/UART_WakeUpFromStopUsingFIFO/Inc/stm32h7xx_it.h          Interrupt handlers header file
  - UART/UART_WakeUpFromStopUsingFIFO/Inc/main.h                  Header for main.c module
  - UART/UART_WakeUpFromStopUsingFIFO/Src/stm32h7xx_it.c          Interrupt handlers
  - UART/UART_WakeUpFromStopUsingFIFO/Src/main.c                  Main program
  - UART/UART_WakeUpFromStopUsingFIFO/Src/stm32h7xx_hal_msp.c     HAL MSP module
  - UART/UART_WakeUpFromStopUsingFIFO/Src/system_stm32h7xx.c      STM32H7xx system source file


@par Hardware and Software environment

  - This example runs on STM32H745xx devices.
    
  - This example has been tested with NUCLEO-H745ZI-Q board and can be
    easily tailored to any other supported device and development board.

  - NUCLEO-H745ZI-Q Set-up
      Connect a USB cable between the ST-Link usb connector CN1
	  and PC todisplay data on the HyperTerminal.

  - Hyperterminal configuration:
    - Data Length = 7 Bits
    - One Stop Bit
    - Odd parity
    - BaudRate = 9600 baud
    - Flow control: None

@par How to use it ? 

In order to make the program work, you must do the following :
 - Open your preferred toolchain
 - Rebuild all files and load your image into target memory
 - Run the example


 */
