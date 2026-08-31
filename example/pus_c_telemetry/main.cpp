// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPack.h>
#include <iostream>
#include <memory>

namespace {
  int fail(const char *operation, const ccsds::Error &error) {
    std::cerr << operation << ": " << error.message() << '\n';
    return error.code() == ccsds::NONE ? 1 : error.code();
  }
}

int main() {
  ccsds::pus::rev_c::TmTailoring tailoring;
  tailoring.timestampPresent = true;
  tailoring.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                   ccsds::time::PFieldMode::Implicit, 4U, 0U};

  ccsds::Packet packet;
  if (const auto result = packet.setPrimaryHeader(ccsds::PrimaryHeader{
        0U, 1U, 0U, 0x456U, ccsds::UNSEGMENTED, 8U, 0U}); !result) {
    return fail("setPrimaryHeader", result.error());
  }
  if (const auto result = packet.setSecondaryHeader(
        std::make_shared<ccsds::pus::rev_c::TmHeader>(
          tailoring, 3U, 25U, 7U, 0x0102U, 5U,
          ccsds::time::CucTime{0x11223344U, 0U})); !result) {
    return fail("setSecondaryHeader", result.error());
  }
  if (const auto result = packet.setApplicationData({0x80U, 0x81U}); !result) {
    return fail("setApplicationData", result.error());
  }

  const auto wireResult = packet.serialize();
  if (!wireResult) return fail("serialize", wireResult.error());
  const auto &wire = wireResult.value();

  // Typed parsing accepts the same optional tailoring when the wire layout needs it.
  ccsds::Packet decoded;
  const auto consumed = decoded.deserializeBounded<ccsds::pus::rev_c::TmHeader>(
    wire, tailoring);
  if (!consumed) return fail("deserializeBounded", consumed.error());

  const auto header = std::static_pointer_cast<const ccsds::pus::rev_c::TmHeader>(
    decoded.getSecondaryHeader());
  if (consumed.value() != wire.size() || !header
      || header->getServiceType() != 3U || header->getServiceSubtype() != 25U
      || header->getMessageTypeCounter() != 7U || header->getDestinationId() != 0x0102U
      || header->getTimeReferenceStatus() != 5U
      || header->getTimestamp() != ccsds::time::CucTime{0x11223344U, 0U}
      || decoded.getDirection() != ccsds::PacketDirection::Telemetry) {
    std::cerr << "PUS-C telemetry did not round-trip\n";
    return 1;
  }

  std::cout << header->getType() << ": " << wire.size() << " bytes\n";
  return 0;
}
