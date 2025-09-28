/**
 ******************************************************************************
 * @file    UART/UART_WakeUpFromStopUsingFIFO/CM7/Src/main.c
 * @author  MCD Application Team
 * @brief   This example shows how to use UART HAL API to wake up the MCU from
 *          STOP mode using FIFO filling level.
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
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "CCSDSPack.h"

/** @addtogroup STM32H7xx_HAL_Examples
 * @{
 */

/** @addtogroup UART_WakeUpFromStopUsingFIFO
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define HAL_TIMEOUT_VALUE 0xFFFFFFFF
#define countof(a) (sizeof(a) / sizeof(*(a)))

/* Private variables ---------------------------------------------------------*/
/* UART handler declaration */
UART_HandleTypeDef UartHandle;
uint8_t HeaderTxBuffer[] = "\r\nUART WakeUp from stop mode using FIFO\r\n";
uint8_t Part1TxBuffer[] =
		"\r\nStarting CCSDS tests: please send a signal.\r\n\n";

uint8_t RxBuffer[16];

/* Private function prototypes -----------------------------------------------*/
static void MPU_Config(void);
static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/

class CustomSecondaryHeader final : public CCSDS::SecondaryHeaderAbstract {
public:
	CustomSecondaryHeader() {
		variableLength = true;
	}
	;

	/**
	 * @brief Constructs a CustomSecondaryHeader object with all fields explicitly set.
	 */
	explicit CustomSecondaryHeader(const std::vector<uint8_t> &data) :
			m_data(data) {
		variableLength = true;
	}
	;

	[[nodiscard]] CCSDS::ResultBool deserialize(
			const std::vector<uint8_t> &data) override {
		m_data = data;
		return true;
	}
	;

	[[nodiscard]] uint16_t getSize() const override {
		return m_data.size();
	}
	[[nodiscard]] std::string getType() const override {
		return m_type;
	}

	[[nodiscard]] std::vector<uint8_t> serialize() const override {
		return m_data;
	}
	;
	void update(CCSDS::DataField *dataField) override {
		m_dataLength = m_data.size();
	}
	CCSDS::ResultBool loadFromConfig(const Config &config) override {
		return true;
	}
	;

private:
	std::vector<uint8_t> m_data { };
	uint16_t m_dataLength = 0;
	const std::string m_type = "CustomSecondaryHeader";
};

void uartPrintHex(const std::vector<uint8_t> &v) {
	char buf[8];
	for (size_t i = 0; i < v.size(); ++i) {
		int n = snprintf(buf, sizeof(buf), "%02X ", v[i]);
		HAL_UART_Transmit(&UartHandle, reinterpret_cast<uint8_t*>(buf), n,
				HAL_MAX_DELAY);
	}
	const char crlf[] = "\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) crlf, sizeof(crlf) - 1,
			HAL_MAX_DELAY);
}

int testCCSDS(void) {
	uint8_t bufferCreate[] = "Stage: Create CCSDS Packet Manager.\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferCreate,
	countof(bufferCreate) - 1, HAL_TIMEOUT_VALUE);
	CCSDS::Manager mgr; // Minimal smoke test: type is visible and linkable

	uint8_t bufferComplete[] = "Stage: Complete.\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferComplete,
	countof(bufferComplete) - 1, HAL_TIMEOUT_VALUE);

	uint8_t bufferCreateTemplate[] = "Stage: Create CCSDS Template Package.\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferCreateTemplate,
	countof(bufferCreateTemplate) - 1, HAL_TIMEOUT_VALUE);

	// generate template packet with new registered secondary header
	CCSDS::Packet templatePacket;
	templatePacket.setPrimaryHeader(
			CCSDS::PrimaryHeader { 1, 2, 1, 55, 0, 0, 0 });

	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferComplete,
	countof(bufferComplete) - 1, HAL_TIMEOUT_VALUE);

	uint8_t bufferCreateCustomSH[] =
			"Stage: Register custom secondary header.\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferCreateCustomSH,
	countof(bufferCreateCustomSH) - 1, HAL_TIMEOUT_VALUE);

	const std::vector<uint8_t> data { 0x77, 0xFA, 0xB, 0x0, 0x0, 0xB, 0x5 };

	if (const auto res = templatePacket.RegisterSecondaryHeader<
			CustomSecondaryHeader>(); !res.has_value()) {
		uint8_t bufferError[] = "Error: Register Secondary header.\r\n";
		HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferError,
		countof(bufferError) - 1, HAL_TIMEOUT_VALUE);
		return res.error().code();
	}
	if (const auto res = templatePacket.setDataFieldHeader(data,
			"CustomSecondaryHeader"); !res.has_value()) {
		uint8_t bufferError[] = "Error: Set CCSDS Packet data field header.\r\n";
		HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferError,
		countof(bufferError) - 1, HAL_TIMEOUT_VALUE);
		return res.error().code();
	}
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferComplete,
	countof(bufferComplete) - 1, HAL_TIMEOUT_VALUE);

	// add template to manager
	uint8_t bufferAddTemplate[] =
			"Stage: Set CCSDS Packet template package.\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferAddTemplate,
	countof(bufferAddTemplate) - 1, HAL_TIMEOUT_VALUE);
	if (const auto res = mgr.setPacketTemplate(templatePacket); !res.has_value()) {
		uint8_t bufferError[] = "Error: Set packet template.\r\n";
		HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferError,
		countof(bufferError) - 1, HAL_TIMEOUT_VALUE);
		return res.error().code();
	}
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferComplete,
	countof(bufferComplete) - 1, HAL_TIMEOUT_VALUE);

	// Add some data to generate packets
	uint8_t bufferAddData[] =
			"Stage: Generate CCSDS packet with application data.\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferAddData,
	countof(bufferAddData) - 1, HAL_TIMEOUT_VALUE);
	mgr.setDataFieldSize(1000);
	if (const auto res = mgr.setApplicationData( { 1, 2, 3 }); !res.has_value()) {
		uint8_t bufferError[] = "Error: set Application data.\r\n";
		HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferError,
		countof(bufferError) - 1, HAL_TIMEOUT_VALUE);
		return res.error().code();
	}
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferComplete,
	countof(bufferComplete) - 1, HAL_TIMEOUT_VALUE);

	// get ccsds packets
	uint8_t bufferGetPacketBuffer[] = "Stage: Get CCSDS Packet buffer.\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferGetPacketBuffer,
	countof(bufferGetPacketBuffer) - 1, HAL_TIMEOUT_VALUE);
	const auto packetsData = mgr.getPacketsBuffer();
	uartPrintHex(packetsData);
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferComplete,
	countof(bufferComplete) - 1, HAL_TIMEOUT_VALUE);

	uint8_t bufferTestPackage[] =
			"Stage: Test Generated package against expected.\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferTestPackage,
	countof(bufferTestPackage) - 1, HAL_TIMEOUT_VALUE);
	const std::vector<uint8_t> expectedPacket { 0x28, 0x37, 0x00, 0x00, 0x00,
			0x0a, 0x77, 0xfa, 0x0b, 0x00, 0x00, 0x0b, 0x05, 0x01, 0x02, 0x03,
			0x3e, 0x97 };

	if (std::equal(packetsData.begin(), packetsData.end(),
			expectedPacket.begin())) {
		uint8_t bufferExpected[] = "Stage: -- Generated packet as expected.\r\n";
		HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferExpected,
		countof(bufferExpected) - 1, HAL_TIMEOUT_VALUE);
	} else {
		uint8_t bufferExpected[] =
				"Stage: -- Generated packet differs from expected.\r\n";
		HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferExpected,
		countof(bufferExpected) - 1, HAL_TIMEOUT_VALUE);
	}
	return 0;
}

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {
	int32_t timeout;
	/* Configure the MPU attributes */
	MPU_Config();

	/* Enable the CPU Cache */
	CPU_CACHE_Enable();
	timeout = 0xFFFF;
	while ((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0))
		;
	if (timeout < 0) {
		Error_Handler();
	}
	/* STM32H7xx HAL library initialization:
	 - Systick timer is configured by default as source of time base, but user
	 can eventually implement his proper time base source (a general purpose
	 timer for example or other time source), keeping in mind that Time base
	 duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
	 handled in milliseconds basis.
	 - Set NVIC Group Priority to 4
	 - Low Level Initialization
	 */
	HAL_Init();

	/* Configure the system clock to 400 MHz */
	SystemClock_Config();

	/* Initialize BSP LEDs */
	BSP_LED_Init(LED1);
	BSP_LED_Init(LED2);
	BSP_LED_Init(LED3);

	/* Turn LED1 on */
	BSP_LED_On(LED1);

	/*##########################################################################*/
	/*##-1- Configure the UART peripheral ######################################*/
	/*##########################################################################*/

	/* Put the USART peripheral in the Asynchronous mode (UART Mode) */
	/* UART configured as follows:
	 - Word Length = 8 Bits (7 data bit + 1 parity bit) :
	 BE CAREFUL : Program 7 data bits + 1 parity bit in PC HyperTerminal
	 - Stop Bit    = One Stop bit
	 - Parity      = Odd
	 - BaudRate    = 9600 baud
	 - Hardware flow control disabled (RTS and CTS signals) */
	UartHandle.Instance = USARTx;
	UartHandle.Init.BaudRate = 9600;
	UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	UartHandle.Init.StopBits = UART_STOPBITS_1;
	UartHandle.Init.Parity = UART_PARITY_ODD;
	UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	UartHandle.Init.Mode = UART_MODE_TX_RX;
	UartHandle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	UartHandle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;

	if (HAL_UART_Init(&UartHandle) != HAL_OK) {
		/* Initialization Error */
		Error_Handler();
	}

	/* Set the RXFIFO threshold */
	HAL_UARTEx_SetRxFifoThreshold(&UartHandle, UART_RXFIFO_THRESHOLD_1_4);

	/* Enable the FIFO mode */
	HAL_UARTEx_EnableFifoMode(&UartHandle);

	/* Output message on hyperterminal */
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &HeaderTxBuffer,
	countof(HeaderTxBuffer) - 1, HAL_TIMEOUT_VALUE);

	/*##########################################################################*/
	/*##-2- Wakeup first step RXFT #############################################*/
	/*##########################################################################*/

	/* Enable MCU wakeup by UART */
	HAL_UARTEx_EnableStopMode(&UartHandle);

	/* Enable the UART RX FIFO threshold interrupt */
	__HAL_UART_ENABLE_IT(&UartHandle, UART_IT_RXFT);

	/* Enable the UART wakeup from stop mode interrupt */
	__HAL_UART_ENABLE_IT(&UartHandle, UART_IT_WUF);

	/* Output message on hyperterminal */
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &Part1TxBuffer,
	countof(Part1TxBuffer) - 1, HAL_TIMEOUT_VALUE);

	/* Put UART peripheral in reception process */
	HAL_UART_Receive_IT(&UartHandle, (uint8_t*) &RxBuffer, 1);

	/* Turn LED1 off */
	BSP_LED_Off(LED1);

	/* Enter STOP mode */
	HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

	/* ... STOP Mode ... */

	while (HAL_UART_GetState(&UartHandle) != HAL_UART_STATE_READY) {
	}

	/* Turn LED1 on */
	BSP_LED_On(LED1);

	/* Disable the UART wakeup from stop mode interrupt */
	__HAL_UART_DISABLE_IT(&UartHandle, UART_IT_WUF);

	/* Disable the UART RX FIFO threshold interrupt */
	__HAL_UART_DISABLE_IT(&UartHandle, UART_IT_RXFT);

	/* Disable UART Stop Mode */
	HAL_UARTEx_DisableStopMode(&UartHandle);

	//test CCSDS library.
	if (testCCSDS() != 0) {
		Error_Handler();
	};

	/* Turn LED1 on */
	BSP_LED_On(LED2);

	uint8_t bufferEnd[] = "\r\nTest End. Reset STM32 to start test again.\r\n";
	HAL_UART_Transmit(&UartHandle, (uint8_t*) &bufferEnd,
	countof(bufferEnd) - 1, HAL_TIMEOUT_VALUE);

	/* Infinite loop */
	while (1) {
		;
	}
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE BYPASS)
 *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
 *            HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
 *            AHB Prescaler                  = 2
 *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
 *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
 *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
 *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
 *            HSE Frequency(Hz)              = 8000000
 *            PLL_M                          = 4
 *            PLL_N                          = 400
 *            PLL_P                          = 2
 *            PLL_Q                          = 4
 *            PLL_R                          = 2
 *            VDD(V)                         = 3.3
 *            Flash Latency(WS)              = 4
 * @param  None
 * @retval None
 */
static void SystemClock_Config(void) {
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	HAL_StatusTypeDef ret = HAL_OK;
	/* The voltage scaling allows optimizing the power consumption when the device is
	 clocked below the maximum system frequency, to update the voltage scaling value
	 regarding system frequency refer to product datasheet.  */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
	}

	/* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 400;
	RCC_OscInitStruct.PLL.PLLFRACN = 0;
	RCC_OscInitStruct.PLL.PLLP = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLQ = 4;

	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
	ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (ret != HAL_OK) {
		Error_Handler();
	}

	/* Select PLL as system clock source and configure  bus clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
			| RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 |
			RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1);

	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
	RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
	ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
	if (ret != HAL_OK) {
		Error_Handler();
	}

}

/**
 * @brief  CPU L1-Cache enable.
 * @param  None
 * @retval None
 */
static void CPU_CACHE_Enable(void) {
	/* Enable I-Cache */
	SCB_EnableICache();

	/* Enable D-Cache */
	SCB_EnableDCache();
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
static void Error_Handler(void) {
	/* Turn LED3 on */
	BSP_LED_On(LED3);
	while (1) {
		;
	}
}

/**
 * @brief  Configure the MPU attributes
 * @param  None
 * @retval None
 */
static void MPU_Config(void) {
	MPU_Region_InitTypeDef MPU_InitStruct;

	/* Disable the MPU */
	HAL_MPU_Disable();

	/* Configure the MPU as Strongly ordered for not defined regions */
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress = 0x00;
	MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
	MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER0;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x87;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/* Enable the MPU */
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while(1) { ; }
}
#endif

/**
 * @}
 */

/**
 * @}
 */

