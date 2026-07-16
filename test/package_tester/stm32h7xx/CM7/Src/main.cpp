// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file main.cpp
 * @brief STM32H745 CM7 hardware-validation harness for the CCSDSPack MCU archive.
 *
 * The board runs the same HAL-independent validation core that is compiled by the
 * generic arm-none-eabi package build. UART and LEDs provide deterministic target
 * results:
 *
 * - `CCSDSPACK_MCU_TEST:PASS` and LED2 on indicate success;
 * - `CCSDSPACK_MCU_TEST:FAIL:<code>` and LED3 on identify the first failed stage.
 */

#include "main.h"
#include "ccsdspack_mcu_test.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
  constexpr std::uint32_t UART_TIMEOUT = HAL_MAX_DELAY;

  UART_HandleTypeDef UartHandle{};
  bool uartReady{false};
  bool ledsReady{false};

  void MPU_Config();
  void SystemClock_Config();
  void CPU_CACHE_Enable();
  [[noreturn]] void Error_Handler();

  void uartWrite(const char *text) {
    if (!uartReady || text == nullptr) return;
    const auto length = std::strlen(text);
    HAL_UART_Transmit(&UartHandle,
                      reinterpret_cast<std::uint8_t *>(const_cast<char *>(text)),
                      static_cast<std::uint16_t>(length),
                      UART_TIMEOUT);
  }

  void reportTestResult(const int result) {
    if (result == CCSDSPackMcuTest::Pass) {
      BSP_LED_On(LED2);
      uartWrite("CCSDSPACK_MCU_TEST:PASS\r\n");
      return;
    }

    BSP_LED_On(LED3);
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "CCSDSPACK_MCU_TEST:FAIL:%d\r\n", result);
    uartWrite(buffer);
  }
} // namespace

int main() {
  std::int32_t timeout = 0xFFFF;

  MPU_Config();
  CPU_CACHE_Enable();

  // On the dual-core STM32H745, wait until the CM4 domain is in reset before
  // configuring the CM7 clocks. This follows the original ST board example.
  while ((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0)) {
  }
  if (timeout < 0) {
    Error_Handler();
  }

  HAL_Init();
  SystemClock_Config();

  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
  ledsReady = true;

  BSP_LED_Off(LED1);
  BSP_LED_Off(LED2);
  BSP_LED_Off(LED3);
  BSP_LED_On(LED1);

  // Standard ST-Link virtual COM settings: 115200 baud, 8 data bits, no parity,
  // one stop bit, no flow control.
  UartHandle.Instance = USARTx;
  UartHandle.Init.BaudRate = 115200;
  UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits = UART_STOPBITS_1;
  UartHandle.Init.Parity = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode = UART_MODE_TX_RX;
  UartHandle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  UartHandle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&UartHandle) != HAL_OK
      || HAL_UARTEx_SetTxFifoThreshold(&UartHandle, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK
      || HAL_UARTEx_SetRxFifoThreshold(&UartHandle, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK
      || HAL_UARTEx_DisableFifoMode(&UartHandle) != HAL_OK) {
    Error_Handler();
  }
  uartReady = true;

  uartWrite("\r\nCCSDSPack STM32H745 CM7 hardware validation\r\n");
  uartWrite("Running packet generation, parsing, CRC, Manager, Validator, PVN, and Idle tests...\r\n");

  const int result = CCSDSPackMcuTest::run();
  reportTestResult(result);

  uartWrite("Reset the board to run the validation again.\r\n");
  BSP_LED_Off(LED1);

  while (true) {
    __WFI();
  }
}

namespace {
  /**
   * @brief Configures the CM7 system clock for 400 MHz from the 8 MHz HSE bypass.
   */
  void SystemClock_Config() {
    RCC_ClkInitTypeDef clock{};
    RCC_OscInitTypeDef oscillator{};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator.HSEState = RCC_HSE_BYPASS;
    oscillator.HSIState = RCC_HSI_OFF;
    oscillator.CSIState = RCC_CSI_OFF;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLM = 4;
    oscillator.PLL.PLLN = 400;
    oscillator.PLL.PLLFRACN = 0;
    oscillator.PLL.PLLP = 2;
    oscillator.PLL.PLLQ = 4;
    oscillator.PLL.PLLR = 2;
    oscillator.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    oscillator.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;

    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) {
      Error_Handler();
    }

    clock.ClockType = RCC_CLOCKTYPE_SYSCLK
                      | RCC_CLOCKTYPE_HCLK
                      | RCC_CLOCKTYPE_D1PCLK1
                      | RCC_CLOCKTYPE_PCLK1
                      | RCC_CLOCKTYPE_PCLK2
                      | RCC_CLOCKTYPE_D3PCLK1;
    clock.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clock.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clock.AHBCLKDivider = RCC_HCLK_DIV2;
    clock.APB3CLKDivider = RCC_APB3_DIV2;
    clock.APB1CLKDivider = RCC_APB1_DIV2;
    clock.APB2CLKDivider = RCC_APB2_DIV2;
    clock.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_4) != HAL_OK) {
      Error_Handler();
    }
  }

  void CPU_CACHE_Enable() {
    SCB_EnableICache();
    SCB_EnableDCache();
  }

  [[noreturn]] void Error_Handler() {
    if (ledsReady) {
      BSP_LED_On(LED3);
    }
    if (uartReady) {
      uartWrite("CCSDSPACK_MCU_TEST:HAL_FAILURE\r\n");
    }
    while (true) {
      __WFI();
    }
  }

  void MPU_Config() {
    MPU_Region_InitTypeDef region{};

    HAL_MPU_Disable();

    region.Enable = MPU_REGION_ENABLE;
    region.BaseAddress = 0x00000000U;
    region.Size = MPU_REGION_SIZE_4GB;
    region.AccessPermission = MPU_REGION_NO_ACCESS;
    region.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    region.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    region.IsShareable = MPU_ACCESS_SHAREABLE;
    region.Number = MPU_REGION_NUMBER0;
    region.TypeExtField = MPU_TEX_LEVEL0;
    region.SubRegionDisable = 0x87;
    region.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

    HAL_MPU_ConfigRegion(&region);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
  }
} // namespace

#ifdef USE_FULL_ASSERT
extern "C" void assert_failed(std::uint8_t *, std::uint32_t) {
  Error_Handler();
}
#endif
