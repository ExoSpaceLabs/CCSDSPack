// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack_mcu_test.h"

extern "C" {
#include <das/das.h>
}

#include <cstddef>
#include <cstdint>

extern "C" std::size_t ccsdspack_h755_heap_capacity_bytes(void);
extern "C" std::size_t ccsdspack_h755_heap_peak_bytes(void);

#ifndef CCSDSPACK_VALIDATION_SOURCE_SHA
#define CCSDSPACK_VALIDATION_SOURCE_SHA "unknown"
#endif
#ifndef CCSDSPACK_VALIDATION_DAS_SHA
#define CCSDSPACK_VALIDATION_DAS_SHA "unknown"
#endif
#ifndef CCSDSPACK_VALIDATION_LIBRARY_SHA256
#define CCSDSPACK_VALIDATION_LIBRARY_SHA256 "unknown"
#endif
#ifndef CCSDSPACK_VALIDATION_PACKAGE_SHA256
#define CCSDSPACK_VALIDATION_PACKAGE_SHA256 "unknown"
#endif

namespace {

das_uart_t g_console = DAS_UART_INVALID;
bool g_console_ready = false;
bool g_leds_ready = false;

std::size_t textLength(const char *text) {
  if (text == nullptr) return 0U;
  std::size_t length = 0U;
  while (text[length] != '\0') ++length;
  return length;
}

void uartWrite(const char *text) {
  if (!g_console_ready || text == nullptr) return;
  const std::size_t length = textLength(text);
  if (length == 0U) return;
  (void)das_uart_write(
      g_console,
      reinterpret_cast<const std::uint8_t *>(text),
      length);
}

void uartLine(const char *text) {
  uartWrite(text);
  uartWrite("\r\n");
}

void uartHex32(std::uint32_t value) {
  static constexpr char hex[] = "0123456789ABCDEF";
  char text[11]{'0', 'x'};
  for (std::size_t index = 0U; index < 8U; ++index) {
    const std::uint32_t shift = static_cast<std::uint32_t>((7U - index) * 4U);
    text[2U + index] = hex[(value >> shift) & 0x0FU];
  }
  text[10] = '\0';
  uartWrite(text);
}

void uartUnsigned(std::uint32_t value) {
  char digits[11]{};
  std::size_t count = 0U;
  do {
    digits[count++] = static_cast<char>('0' + (value % 10U));
    value /= 10U;
  } while (value != 0U && count < sizeof(digits));

  while (count > 0U) {
    const char digit[2]{digits[--count], '\0'};
    uartWrite(digit);
  }
}

void uartKeyValue(const char *key, const char *value) {
  uartWrite(key);
  uartWrite(":");
  uartLine(value);
}

void uartKeyValueUnsigned(const char *key, std::uint32_t value) {
  uartWrite(key);
  uartWrite(":");
  uartUnsigned(value);
  uartWrite("\r\n");
}

void progressReporter(const char *message) {
  uartLine(message);
}

[[noreturn]] void haltForever() {
  for (;;) {
    __asm volatile("wfi");
  }
}

[[noreturn]] void fault(const char *name) {
  constexpr std::uintptr_t shcsrAddress = UINT32_C(0xE000ED24);
  constexpr std::uintptr_t cfsrAddress = UINT32_C(0xE000ED28);
  constexpr std::uintptr_t hfsrAddress = UINT32_C(0xE000ED2C);
  constexpr std::uintptr_t mmfarAddress = UINT32_C(0xE000ED34);
  constexpr std::uintptr_t bfarAddress = UINT32_C(0xE000ED38);
  constexpr std::uintptr_t cpacrAddress = UINT32_C(0xE000ED88);

  if (g_leds_ready) {
    (void)das_board_led_set(DAS_BOARD_LED_GREEN, false);
    (void)das_board_led_set(DAS_BOARD_LED_YELLOW, false);
    (void)das_board_led_set(DAS_BOARD_LED_RED, true);
  }
  uartWrite("FAULT:");
  uartLine(name);

  const auto printFaultRegister = [](const char *name, std::uintptr_t address) {
    uartWrite(name);
    uartWrite(":");
    uartHex32(*reinterpret_cast<volatile const std::uint32_t *>(address));
    uartWrite("\r\n");
  };

  printFaultRegister("FAULT_SHCSR", shcsrAddress);
  printFaultRegister("FAULT_CFSR", cfsrAddress);
  printFaultRegister("FAULT_HFSR", hfsrAddress);
  printFaultRegister("FAULT_MMFAR", mmfarAddress);
  printFaultRegister("FAULT_BFAR", bfarAddress);
  printFaultRegister("FAULT_CPACR", cpacrAddress);
  uartLine("CCSDSPACK_HARDWARE_TEST:FAULT");
  if (g_console_ready) (void)das_uart_flush(g_console);
  haltForever();
}

void printRuntimeInformation() {
  std::uint32_t core_hz = 0U;
  std::uint32_t baud_hz = 0U;

  uartLine("=== CCSDSPack STM32H755 hardware validation ===");
  uartKeyValue("BOARD", "NUCLEO-H755ZI-Q");
  uartKeyValue("CORE", "Cortex-M7");
  uartKeyValue("TRANSPORT", "DAS UART / ST-LINK VCP");
  uartKeyValue("UART_FORMAT", "115200 8N1");
  uartKeyValue("CCSDSPACK_SOURCE_SHA", CCSDSPACK_VALIDATION_SOURCE_SHA);
  uartKeyValue("CCSDSPACK_PACKAGE_SHA256", CCSDSPACK_VALIDATION_PACKAGE_SHA256);
  uartKeyValue("CCSDSPACK_LIBRARY_SHA256", CCSDSPACK_VALIDATION_LIBRARY_SHA256);
  uartKeyValue("DAS_SHA", CCSDSPACK_VALIDATION_DAS_SHA);
  uartKeyValue("COMPILER", __VERSION__);
  uartKeyValueUnsigned("CPP_STANDARD", static_cast<std::uint32_t>(__cplusplus));

  if (das_clock_get_core_frequency(&core_hz) == DAS_OK) {
    uartKeyValueUnsigned("CORE_HZ", core_hz);
  } else {
    uartKeyValue("CORE_HZ", "unavailable");
  }

  if (das_uart_get_baud_rate(g_console, &baud_hz) == DAS_OK) {
    uartKeyValueUnsigned("UART_EFFECTIVE_BAUD", baud_hz);
  } else {
    uartKeyValue("UART_EFFECTIVE_BAUD", "unavailable");
  }

  uartLine("ACCEPTANCE:Packet, Manager, CRC16, raw buffers, Validator, PUS-C, PVN, Idle");
}

} // namespace

extern "C" [[noreturn]] void HardFault_Handler(void) {
  fault("HardFault");
}
extern "C" [[noreturn]] void MemManage_Handler(void) {
  fault("MemManage");
}
extern "C" [[noreturn]] void BusFault_Handler(void) {
  fault("BusFault");
}
extern "C" [[noreturn]] void UsageFault_Handler(void) {
  fault("UsageFault");
}

int main() {
  if (das_clock_set_frequency(UINT32_C(400000000)) != DAS_OK) {
    haltForever();
  }

  if (das_board_led_init_all(false) == DAS_OK) {
    g_leds_ready = true;
    (void)das_board_led_set(DAS_BOARD_LED_GREEN, true);
  }

  das_uart_config_t uart_config{};
  uart_config.baud_rate = UINT32_C(115200);
  uart_config.data_bits = DAS_UART_DATA_BITS_8;
  uart_config.parity = DAS_UART_PARITY_NONE;
  uart_config.stop_bits = DAS_UART_STOP_BITS_1;

  if (das_board_uart_init(
          DAS_BOARD_UART_STLINK_VCP,
          &uart_config,
          &g_console) != DAS_OK ||
      !das_uart_is_valid(g_console)) {
    if (g_leds_ready) {
      (void)das_board_led_set(DAS_BOARD_LED_GREEN, false);
      (void)das_board_led_set(DAS_BOARD_LED_RED, true);
    }
    haltForever();
  }

  g_console_ready = true;
  printRuntimeInformation();
  uartLine("CCSDSPACK_HARDWARE_TEST:BEGIN");

  const int result = CCSDSPackMcuTest::run(progressReporter);
  uartKeyValueUnsigned("RESULT_CODE", static_cast<std::uint32_t>(result));
  uartKeyValue("RESULT_NAME", CCSDSPackMcuTest::resultCodeName(result));
  uartKeyValueUnsigned(
      "HEAP_CAPACITY_BYTES",
      static_cast<std::uint32_t>(ccsdspack_h755_heap_capacity_bytes()));
  uartKeyValueUnsigned(
      "HEAP_PEAK_BYTES",
      static_cast<std::uint32_t>(ccsdspack_h755_heap_peak_bytes()));

  if (result == CCSDSPackMcuTest::Pass) {
    if (g_leds_ready) {
      (void)das_board_led_set(DAS_BOARD_LED_GREEN, false);
      (void)das_board_led_set(DAS_BOARD_LED_YELLOW, true);
    }
    uartLine("CCSDSPACK_HARDWARE_TEST:PASS");
  } else {
    if (g_leds_ready) {
      (void)das_board_led_set(DAS_BOARD_LED_GREEN, false);
      (void)das_board_led_set(DAS_BOARD_LED_RED, true);
    }
    uartWrite("CCSDSPACK_HARDWARE_TEST:FAIL:");
    uartUnsigned(static_cast<std::uint32_t>(result));
    uartWrite(":");
    uartLine(CCSDSPackMcuTest::resultCodeName(result));
  }

  uartLine("CCSDSPACK_HARDWARE_TEST:END");
  (void)das_uart_flush(g_console);
  haltForever();
}
