// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_MCU_TEST_H
#define CCSDSPACK_MCU_TEST_H

#ifndef CCSDS_MCU
#error "Define CCSDS_MCU when compiling the STM32 validation application"
#endif

#include "../../../hardware/ccsdspack_hardware_test.h"

namespace CCSDSPackMcuTest = CCSDSPackHardwareTest;

#endif // CCSDSPACK_MCU_TEST_H
