// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <das/das.h>

#include "ccsdspack_hardware_test.h"
#include "validation_build_info.h"

#include <cstddef>
#include <cstdint>

namespace {
das_uart_t g_console = DAS_UART_INVALID;
std::uint8_t *g_heap_current = nullptr;
std::size_t g_heap_peak = 0U;

extern "C" std::uint8_t __HeapBase;
extern "C" std::uint8_t __HeapLimit;

std::size_t textLength(const char *text) {
  if (text == nullptr) return 0U;
  std::size_t length = 0U;
  while (text[length] != '\0') ++length;
  return length;
}

void uartWrite(const char *text) {
  if (!das_uart_is_valid(g_console) || text == nullptr) return;
  (void)das_uart_write(
    g_console,
    reinterpret_cast<const std::uint8_t *>(text),
    textLength(text));
}

void uartLine(const char *key, const char *value) {
  uartWrite(key);
  uartWrite(value != nullptr ? value : "unknown");
  uartWrite("\r\n");
}

void uartUnsigned(std::uint32_t value) {
  char buffer[11]{};
  std::size_t index = sizeof(buffer);
  do {
    buffer[--index] = static_cast<char>('0' + (value % 10U));
    value /= 10U;
  } while (value != 0U);
  (void)das_uart_write(
    g_console,
    reinterpret_cast<const std::uint8_t *>(&buffer[index]),
    sizeof(buffer) - index);
}

void uartUnsignedLine(const char *key, const std::uint32_t value) {
  uartWrite(key);
  uartUnsigned(value);
  uartWrite("\r\n");
}

void reportProgress(const char *message) {
  uartWrite("CCSDSPACK_");
  uartWrite(message);
  uartWrite("\r\n");
}

[[noreturn]] void stopForever() {
  for (;;) {
    __asm volatile("wfi");
  }
}

[[noreturn]] void fault(const char *name) {
  (void)das_board_led_set(DAS_BOARD_LED_YELLOW, false);
  (void)das_board_led_set(DAS_BOARD_LED_GREEN, false);
  (void)das_board_led_set(DAS_BOARD_LED_RED, true);
  uartWrite("CCSDSPACK_HARDWARE_TEST:FAULT:");
  uartWrite(name);
  uartWrite("\r\n");
  if (das_uart_is_valid(g_console)) (void)das_uart_flush(g_console);
  stopForever();
}

void printEnvironment() {
  std::uint32_t coreHz = 0U;
  std::uint32_t uartHz = 0U;
  (void)das_clock_get_core_frequency(&coreHz);
  (void)das_uart_get_baud_rate(g_console, &uartHz);

  uartWrite("\r\n");
  uartWrite("CCSDSPACK_HARDWARE_TEST:BEGIN\r\n");
  uartLine("CCSDSPACK_VERSION:", CCSDSPACK_VALIDATION_VERSION);
  uartLine("CCSDSPACK_SOURCE_SHA:", CCSDSPACK_VALIDATION_SOURCE_SHA);
  uartLine("CCSDSPACK_PACKAGE_SHA256:", CCSDSPACK_VALIDATION_PACKAGE_SHA256);
  uartLine("CCSDSPACK_LIBRARY_SHA256:", CCSDSPACK_VALIDATION_LIBRARY_SHA256);
  uartLine("CCSDSPACK_DAS_SHA:", CCSDSPACK_VALIDATION_DAS_SHA);
  uartLine("CCSDSPACK_BOARD:", "NUCLEO-H755ZI-Q");
  uartLine("CCSDSPACK_CORE:", "Cortex-M7");
  uartLine("CCSDSPACK_TRANSPORT:", "DAS ST-LINK VCP / USART3 PD8-PD9");
  uartLine("CCSDSPACK_UART_FORMAT:", "115200 8N1");
  uartLine("CCSDSPACK_COMPILER:", __VERSION__);
  uartLine("CCSDSPACK_CPP_STANDARD:", "C++17");
  uartLine("CCSDSPACK_EXCEPTIONS:", "disabled");
  uartLine("CCSDSPACK_RTTI:", "disabled");
  uartUnsignedLine("CCSDSPACK_CORE_HZ:", coreHz);
  uartUnsignedLine("CCSDSPACK_UART_EFFECTIVE_BAUD:", uartHz);
  uartUnsignedLine(
    "CCSDSPACK_HEAP_CAPACITY_BYTES:",
    static_cast<std::uint32_t>(&__HeapLimit - &__HeapBase));
  uartWrite("CCSDSPACK_TEST_SUITE:Packet+PEC+PUS-C+Validator+raw-buffer+Manager+PVN+Idle\r\n");
}

void printMemorySummary() {
  const auto used = g_heap_current == nullptr
    ? 0U
    : static_cast<std::size_t>(g_heap_current - &__HeapBase);
  uartUnsignedLine("CCSDSPACK_HEAP_USED_BYTES:", static_cast<std::uint32_t>(used));
  uartUnsignedLine("CCSDSPACK_HEAP_PEAK_BYTES:", static_cast<std::uint32_t>(g_heap_peak));
}
} // namespace

extern "C" void *_sbrk(std::ptrdiff_t increment) {
  if (g_heap_current == nullptr) g_heap_current = &__HeapBase;

  const auto aligned = increment >= 0
    ? (increment + static_cast<std::ptrdiff_t>(7)) & ~static_cast<std::ptrdiff_t>(7)
    : increment;

  std::uint8_t *next = g_heap_current + aligned;
  if (next < &__HeapBase || next > &__HeapLimit) {
    return reinterpret_cast<void *>(-1);
  }

  void *previous = g_heap_current;
  g_heap_current = next;
  const auto used = static_cast<std::size_t>(g_heap_current - &__HeapBase);
  if (used > g_heap_peak) g_heap_peak = used;
  return previous;
}

extern "C" void HardFault_Handler(void) { fault("HardFault"); }
extern "C" void MemManage_Handler(void) { fault("MemManage"); }
extern "C" void BusFault_Handler(void) { fault("BusFault"); }
extern "C" void UsageFault_Handler(void) { fault("UsageFault"); }
extern "C" void __cxa_pure_virtual(void) { fault("PureVirtual"); }

int main() {
  if (das_clock_set_frequency(UINT32_C(400000000)) != DAS_OK) {
    stopForever();
  }

  const das_uart_config_t uartConfig{
    UINT32_C(115200),
    DAS_UART_DATA_BITS_8,
    DAS_UART_PARITY_NONE,
    DAS_UART_STOP_BITS_1
  };
  if (das_board_uart_init(DAS_BOARD_UART_STLINK_VCP, &uartConfig, &g_console) != DAS_OK) {
    stopForever();
  }

  (void)das_board_led_init_all(false);
  (void)das_board_led_set(DAS_BOARD_LED_YELLOW, true);

  printEnvironment();

  const int result = CCSDSPackHardwareTest::run(&reportProgress);
  printMemorySummary();

  uartUnsignedLine("CCSDSPACK_RESULT_CODE:", static_cast<std::uint32_t>(result));
  uartLine("CCSDSPACK_RESULT_NAME:", CCSDSPackHardwareTest::resultName(result));

  if (result == CCSDSPackHardwareTest::Pass) {
    (void)das_board_led_set(DAS_BOARD_LED_YELLOW, false);
    (void)das_board_led_set(DAS_BOARD_LED_GREEN, true);
    uartWrite("CCSDSPACK_HARDWARE_TEST:PASS\r\n");
  } else {
    (void)das_board_led_set(DAS_BOARD_LED_YELLOW, false);
    (void)das_board_led_set(DAS_BOARD_LED_RED, true);
    uartWrite("CCSDSPACK_HARDWARE_TEST:FAIL:");
    uartUnsigned(static_cast<std::uint32_t>(result));
    uartWrite(":");
    uartWrite(CCSDSPackHardwareTest::resultName(result));
    uartWrite("\r\n");
  }

  (void)das_uart_flush(g_console);
  stopForever();
}
