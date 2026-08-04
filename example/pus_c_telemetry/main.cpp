// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPack.h>
#include <iostream>
#include <memory>
#include <vector>

namespace {
  int fail(const char *operation, const CCSDS::Error &error) {
    std::cerr << operation << ": " << error.message() << '\n';
    return error.code() == CCSDS::NONE ? 1 : error.code();
  }
}

int main() {
  auto profile = CCSDS::makePusProfile(
    CCSDS::PusRevision::C, CCSDS::PacketDirection::Telemetry);
  profile.destinationIdOctets = 2U;
  profile.telemetryTimestampPresent = true;
  profile.telemetryTimeCode = CCSDS::TimeCodeFormat::Cuc;
  profile.telemetryTimeCodeOctets = 4U;

  CCSDS::Packet packet;
  if (const auto result = packet.setPrimaryHeader(CCSDS::PrimaryHeader{
        0U, 0U, 0U, 0x456U, CCSDS::UNSEGMENTED, 8U, 0U}); !result) {
    return fail("setPrimaryHeader", result.error());
  }
  if (const auto result = packet.setMissionProfile(profile); !result) {
    return fail("setMissionProfile", result.error());
  }
  if (const auto result = packet.setSecondaryHeader(
        std::make_shared<CCSDS::PusCTmHeader>(
          profile, 3U, 25U, 7U, 0x0102U, 5U,
          std::vector<std::uint8_t>{0x11U, 0x22U, 0x33U, 0x44U})); !result) {
    return fail("setSecondaryHeader", result.error());
  }
  if (const auto result = packet.setApplicationData({0x80U, 0x81U}); !result) {
    return fail("setApplicationData", result.error());
  }

  const auto wire = packet.serialize();
  CCSDS::Packet decoded;
  if (const auto result = decoded.setMissionProfile(profile); !result) {
    return fail("setMissionProfile decoder", result.error());
  }
  const auto consumed = decoded.deserializeBounded(wire, "PUS:revC:TM");
  if (!consumed) return fail("deserializeBounded", consumed.error());

  const auto header = std::dynamic_pointer_cast<const CCSDS::PusCTmHeader>(
    decoded.getSecondaryHeader());
  if (consumed.value() != wire.size() || !header
      || header->getServiceType() != 3U || header->getServiceSubtype() != 25U
      || header->getMessageTypeCounter() != 7U || header->getDestinationId() != 0x0102U
      || header->getTimeReferenceStatus() != 5U
      || header->getTimestamp()
         != std::vector<std::uint8_t>({0x11U, 0x22U, 0x33U, 0x44U})) {
    std::cerr << "PUS-C telemetry did not round-trip\n";
    return 1;
  }

  std::cout << header->getType() << ": " << wire.size() << " bytes\n";
  return 0;
}
