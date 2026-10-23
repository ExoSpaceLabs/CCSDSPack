// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack_mcu_test.h"
#include <string>
#include <type_traits>
#include <utility>

// MCU clients must not see methods whose hosted filesystem helpers are not
// compiled into the bare-metal archive.
template <typename T, typename = void>
struct HasHostedRead : std::false_type {};

template <typename T>
struct HasHostedRead<T, std::void_t<decltype(std::declval<T &>().read(
    std::declval<const std::string &>()))>> : std::true_type {};

template <typename T, typename = void>
struct HasHostedWrite : std::false_type {};

template <typename T>
struct HasHostedWrite<T, std::void_t<decltype(std::declval<T &>().write(
    std::declval<const std::string &>()))>> : std::true_type {};

static_assert(!HasHostedRead<ccsds::Manager>::value,
              "MCU Manager must not expose hosted file reads");
static_assert(!HasHostedWrite<ccsds::Manager>::value,
              "MCU Manager must not expose hosted file writes");

// Compiled and relocatably linked by the generic arm-none-eabi package build.
// The exact same acceptance body is then executed on the physical STM32H755.
extern "C" int ccsdspack_mcu_compile_probe() {
  return CCSDSPackMcuTest::run();
}
