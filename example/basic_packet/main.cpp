// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPack.h>
#include <iostream>
#include <vector>

namespace {
  int fail(const char *operation, const ccsds::Error &error) {
    std::cerr << operation << ": " << error.message() << '\n';
    return error.code() == ccsds::NONE ? 1 : error.code();
  }
}

int main() {
  ccsds::Packet packet;
  packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

  if (const auto result = packet.setPrimaryHeader(ccsds::PrimaryHeader{
        0U, 0U, 0U, 0x123U, ccsds::UNSEGMENTED, 42U, 0U}); !result) {
    return fail("setPrimaryHeader", result.error());
  }
  if (const auto result = packet.setApplicationData({0x10U, 0x20U, 0x30U}); !result) {
    return fail("setApplicationData", result.error());
  }

  const auto wireResult = packet.serialize();
  if (!wireResult) return fail("serialize", wireResult.error());
  const auto &wire = wireResult.value();

  ccsds::Packet decoded;
  decoded.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
  const auto consumed = decoded.deserializeBounded(wire);
  if (!consumed) return fail("deserializeBounded", consumed.error());

  if (consumed.value() != wire.size()
      || decoded.getPrimaryHeader().getAPID() != 0x123U
      || decoded.getPrimaryHeader().getSequenceCount() != 42U
      || decoded.getApplicationDataBytes() != std::vector<std::uint8_t>({0x10U, 0x20U, 0x30U})) {
    std::cerr << "decoded packet differs from the source packet\n";
    return 1;
  }

  std::cout << "Generic packet: " << wire.size() << " bytes\n";
  return 0;
}
