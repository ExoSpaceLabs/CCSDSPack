// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack_mcu_test.h"

// This target is compiled, but not executed, by the generic arm-none-eabi build.
// It catches public-header, CCSDS_MCU, C++17, and consumer API regressions before
// the same validation core is built and run inside the STM32CubeIDE application.
extern "C" int ccsdspack_mcu_compile_probe() {
  const int baseResult = CCSDSPackMcuTest::run();
  if (baseResult != 0) return baseResult;

  // Representative raw transport buffer. The packet is generic CCSDS, CRC-free,
  // APID 0x123, sequence count 7, with two application-data bytes AA 55.
  const std::uint8_t wire[]{
    0x01, 0x23, 0xC0, 0x07, 0x00, 0x01, 0xAA, 0x55
  };

  const auto declared = ccsds::buffer::declaredPacketSize(wire, 6U);
  if (!declared || declared.value() != sizeof(wire)) return 100;

  ccsds::Packet packet;
  packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
  const auto consumed = ccsds::buffer::deserializeBounded(packet, wire, sizeof(wire));
  if (!consumed || consumed.value() != sizeof(wire)) return 101;
  if (packet.getApplicationDataBytes()
      != std::vector<std::uint8_t>({0xAA, 0x55})) return 102;

  return 0;
}
