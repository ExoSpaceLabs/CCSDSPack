// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPack.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class MissionSecondaryHeader final : public CCSDS::SecondaryHeaderAbstract {
public:
  [[nodiscard]] CCSDS::ResultBool deserialize(
      const std::vector<std::uint8_t> &data) override {
    if (data.size() != getSize()) {
      return CCSDS::Error{CCSDS::INVALID_SECONDARY_HEADER_DATA,
                          "MissionSecondaryHeader requires two bytes"};
    }
    m_data = data;
    return true;
  }

  void update(CCSDS::DataField *) override {}
  [[nodiscard]] std::uint16_t getSize() const override { return 2U; }
  [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }
  [[nodiscard]] std::string getType() const override { return "MissionSecondaryHeader"; }
#ifndef CCSDS_MCU
  CCSDS::ResultBool loadFromConfig(const Config &) override { return true; }
#endif

private:
  std::vector<std::uint8_t> m_data{0U, 0U};
};

namespace {
  int fail(const char *operation, const CCSDS::Error &error) {
    std::cerr << operation << ": " << error.message() << '\n';
    return error.code() == CCSDS::NONE ? 1 : error.code();
  }
}

int main() {
  CCSDS::Packet packet;
  if (const auto result = packet.setPrimaryHeader(CCSDS::PrimaryHeader{
        0U, 0U, 0U, 0x321U, CCSDS::UNSEGMENTED, 0U, 0U}); !result) {
    return fail("setPrimaryHeader", result.error());
  }
  if (const auto result = packet.RegisterSecondaryHeader<MissionSecondaryHeader>(); !result) {
    return fail("RegisterSecondaryHeader", result.error());
  }
  if (const auto result = packet.setSecondaryHeader(
        std::vector<std::uint8_t>{0xA5U, 0x5AU}, "MissionSecondaryHeader"); !result) {
    return fail("setSecondaryHeader", result.error());
  }
  if (const auto result = packet.setApplicationData({0x01U, 0x02U}); !result) {
    return fail("setApplicationData", result.error());
  }

  const auto wireResult = packet.serialize();
  if (!wireResult) return fail("serialize", wireResult.error());
  const auto &wire = wireResult.value();
  CCSDS::Packet decoded;
  if (const auto result = decoded.RegisterSecondaryHeader<MissionSecondaryHeader>(); !result) {
    return fail("RegisterSecondaryHeader decoder", result.error());
  }
  const auto consumed = decoded.deserializeBounded(wire, "MissionSecondaryHeader");
  if (!consumed) return fail("deserializeBounded", consumed.error());

  const auto secondaryHeader = decoded.getSecondaryHeader();
  if (consumed.value() != wire.size() || !secondaryHeader
      || secondaryHeader->serialize() != std::vector<std::uint8_t>({0xA5U, 0x5AU})) {
    std::cerr << "custom secondary header did not round-trip\n";
    return 1;
  }

  std::cout << "Custom secondary header: " << secondaryHeader->getType() << '\n';
  return 0;
}
