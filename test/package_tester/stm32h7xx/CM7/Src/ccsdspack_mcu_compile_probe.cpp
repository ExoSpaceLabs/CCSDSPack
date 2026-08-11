// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack_mcu_test.h"

// This target is compiled, but not executed, by the generic arm-none-eabi build.
// It catches public-header, CCSDS_MCU, C++17, and consumer API regressions before
// the same validation core is built and run inside the STM32CubeIDE application.
extern "C" int ccsdspack_mcu_compile_probe() {
  const int baseResult = CCSDSPackMcuTest::run();
  if (baseResult != 0) return baseResult;

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

  ccsds::Packet pusSource;
  if (!pusSource.setPrimaryHeader(ccsds::PrimaryHeader{
        0U, 0U, 0U, 0x42U, ccsds::UNSEGMENTED, 0U, 0U})) return 103;
  if (!pusSource.setSecondaryHeader(
        std::make_shared<ccsds::pus::rev_c::TcHeader>(
          17U, 1U, 0x1234U, 0x09U))) return 104;
  if (!pusSource.setApplicationData({0xAAU, 0x55U})) return 105;
  const auto pusWire = pusSource.serialize();
  if (!pusWire) return 106;

  ccsds::Packet pusDecoded;
  const auto pusConsumed = ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TcHeader>(
    pusDecoded, pusWire.value().data(), pusWire.value().size());
  if (!pusConsumed || pusConsumed.value() != pusWire.value().size()) return 107;
  if (pusDecoded.getDirection() != ccsds::PacketDirection::Telecommand) return 108;

  return 0;
}
