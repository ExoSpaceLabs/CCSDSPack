// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPack.h>
#include <iostream>
#include <memory>
#include <vector>

namespace {
  int fail(const char *operation, const ccsds::Error &error) {
    std::cerr << operation << ": " << error.message() << '\n';
    return error.code() == ccsds::NONE ? 1 : error.code();
  }
}

int main() {
  auto profile = ccsds::pus::makeProfile(
    ccsds::pus::Revision::C, ccsds::pus::Direction::Telecommand);
  profile.sourceIdOctets = 2U;

  ccsds::Packet packet;
  if (const auto result = packet.setPrimaryHeader(ccsds::PrimaryHeader{
        0U, 1U, 0U, 0x123U, ccsds::UNSEGMENTED, 7U, 0U}); !result) {
    return fail("setPrimaryHeader", result.error());
  }
  if (const auto result = packet.setMissionProfile(profile); !result) {
    return fail("setMissionProfile", result.error());
  }
  if (const auto result = packet.setSecondaryHeader(
        std::make_shared<ccsds::pus::rev_c::TcHeader>(profile, 17U, 1U, 0x1234U, 0x09U)); !result) {
    return fail("setSecondaryHeader", result.error());
  }
  if (const auto result = packet.setApplicationData({0x60U, 0x70U}); !result) {
    return fail("setApplicationData", result.error());
  }

  const auto wireResult = packet.serialize();
  if (!wireResult) return fail("serialize", wireResult.error());
  const auto &wire = wireResult.value();
  ccsds::Packet decoded;
  if (const auto result = decoded.setMissionProfile(profile); !result) {
    return fail("setMissionProfile decoder", result.error());
  }
  const auto consumed = decoded.deserializeBounded(wire, "PUS:revC:TC");
  if (!consumed) return fail("deserializeBounded", consumed.error());

  const auto header = std::dynamic_pointer_cast<const ccsds::pus::rev_c::TcHeader>(
    decoded.getSecondaryHeader());
  if (consumed.value() != wire.size() || !header
      || header->getServiceType() != 17U || header->getServiceSubtype() != 1U
      || header->getSourceId() != 0x1234U || header->getAcknowledgementFlags() != 0x09U) {
    std::cerr << "PUS-C telecommand did not round-trip\n";
    return 1;
  }

  std::cout << header->getType() << ": " << wire.size() << " bytes\n";
  return 0;
}
