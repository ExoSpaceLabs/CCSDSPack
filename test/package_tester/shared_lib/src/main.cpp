// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSPack.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class CustomSecondaryHeader final : public CCSDS::SecondaryHeaderAbstract {
public:
  CustomSecondaryHeader() { variableLength = true; }

  explicit CustomSecondaryHeader(const std::vector<std::uint8_t> &data) : m_data(data) {
    variableLength = true;
  }

  [[nodiscard]] CCSDS::ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
    m_data = data;
    return true;
  }

  [[nodiscard]] std::uint16_t getSize() const override {
    return static_cast<std::uint16_t>(m_data.size());
  }

  [[nodiscard]] std::string getType() const override { return "CustomSecondaryHeader"; }
  [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }

  void update(CCSDS::DataField *) override {
    m_dataLength = static_cast<std::uint16_t>(m_data.size());
  }

  CCSDS::ResultBool loadFromConfig(const Config &) override { return true; }

private:
  std::vector<std::uint8_t> m_data{};
  std::uint16_t m_dataLength{};
};

namespace {
  int fail(const std::string &stage, const std::string &message) {
    std::cerr << "[external consumer] " << stage << ": " << message << '\n';
    return 1;
  }

  int failResult(const std::string &stage, const CCSDS::Error &error) {
    std::cerr << "[external consumer] " << stage << ": " << error.message()
              << " (code " << error.code() << ")\n";
    return error.code() == 0 ? 1 : error.code();
  }
}

int main() {
  CCSDS::Packet templatePacket;
  if (const auto result = templatePacket.setPrimaryHeader(CCSDS::PrimaryHeader{
        0, 1, 0, 0x123, CCSDS::UNSEGMENTED, 5, 0
      }); !result) {
    return failResult("set primary header", result.error());
  }

  if (const auto result = templatePacket.RegisterSecondaryHeader<CustomSecondaryHeader>(); !result) {
    return failResult("register custom secondary header", result.error());
  }

  const std::vector<std::uint8_t> secondaryHeader{0x77, 0xFA, 0x0B, 0x00, 0x00, 0x0B, 0x05};
  if (const auto result = templatePacket.setDataFieldHeader(
        secondaryHeader, "CustomSecondaryHeader"); !result) {
    return failResult("set custom secondary header", result.error());
  }

  CCSDS::Manager manager;
  if (const auto result = manager.setPacketTemplate(templatePacket); !result) {
    return failResult("set Manager template", result.error());
  }
  manager.setAutoValidateEnable(false);
  manager.setDataFieldSize(1000);

  if (manager.getSequenceCount() != 5U || !manager.getAutoSequenceCountEnable()) {
    return fail("sequence configuration", "template sequence state was not inherited");
  }

  if (const auto result = manager.setApplicationData({0x01, 0x02, 0x03}); !result) {
    return failResult("generate packet", result.error());
  }

  const std::vector<std::uint8_t> expectedPacket{
    0x19, 0x23, 0xC0, 0x05, 0x00, 0x0B,
    0x77, 0xFA, 0x0B, 0x00, 0x00, 0x0B, 0x05,
    0x01, 0x02, 0x03,
    0xB7, 0x45
  };

  const auto packetsData = manager.getPacketsBuffer();
  if (packetsData != expectedPacket) {
    return fail("wire vector", "generated v1.2 packet bytes differ from the expected vector");
  }
  if (manager.getSequenceCount() != 6U || manager.getTotalPackets() != 1U) {
    return fail("automatic sequence count", "Manager did not consume exactly one sequence count");
  }

  std::vector<std::uint8_t> templateBytes;
  if (const auto result = manager.getPacketTemplate(); result) {
    templateBytes = result.value();
  } else {
    return failResult("legacy getPacketTemplate", result.error());
  }
  if (templateBytes.empty()) {
    return fail("legacy getPacketTemplate", "serialized template is empty");
  }

  std::vector<std::uint8_t> indexedPacket;
  if (const auto result = manager.getPacketBufferAtIndex(0); result) {
    indexedPacket = result.value();
  } else {
    return failResult("legacy getPacketBufferAtIndex", result.error());
  }
  if (indexedPacket != expectedPacket) {
    return fail("legacy getPacketBufferAtIndex", "indexed packet differs from stream bytes");
  }

  CCSDS::Packet decoded;
  if (const auto result = decoded.RegisterSecondaryHeader<CustomSecondaryHeader>(); !result) {
    return failResult("register decoder secondary header", result.error());
  }
  const auto consumed = decoded.deserializeBounded(
    packetsData, "CustomSecondaryHeader", static_cast<std::int32_t>(secondaryHeader.size()));
  if (!consumed) {
    return failResult("bounded decode", consumed.error());
  }
  if (consumed.value() != packetsData.size()) {
    return fail("bounded decode", "consumed-byte count differs from packet size");
  }

  const CCSDS::Packet &view = decoded;
  const auto &header = view.getPrimaryHeader();
  if (header.getVersionNumber() != 0U
      || header.getType() != 1U
      || header.getDataFieldHeaderFlag() != 1U
      || header.getAPID() != 0x123U
      || header.getSequenceFlags() != CCSDS::UNSEGMENTED
      || header.getSequenceCount() != 5U
      || view.getDataFieldHeaderBytes() != secondaryHeader
      || view.getApplicationDataBytes() != std::vector<std::uint8_t>({0x01, 0x02, 0x03})
      || view.getCRC() != 0xB745U) {
    return fail("decoded fields", "decoded packet does not match the v1.2 logical fields");
  }

  // Exercise legacy observation APIs from an external translation unit.
  if (decoded.getPrimaryHeader64bit() == 0U
      || decoded.getFullPacketLength() != expectedPacket.size()
      || !decoded.getDataFieldHeaderFlag()
      || decoded.getCRCVectorBytes() != std::vector<std::uint8_t>({0xB7, 0x45})) {
    return fail("legacy packet API", "legacy getters returned unexpected values");
  }
  (void)decoded.getDataField();
  (void)decoded.getPrimaryHeader();

  CCSDS::Validator validator(templatePacket);
  validator.configure(true, false, true);
  if (!validator.validate(decoded)) {
    return fail("Validator", "valid packet was rejected against its template identifier");
  }

  CCSDS::Packet mismatchedIdentifier = decoded;
  if (const auto result = mismatchedIdentifier.getPrimaryHeader().setType(0U); !result) {
    return failResult("prepare mismatched identifier", result.error());
  }
  if (manager.addPacket(mismatchedIdentifier)) {
    return fail("Manager identifier", "packet with a different identifier was accepted");
  }

  CCSDS::Packet crcDisabled;
  crcDisabled.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
  if (const auto result = crcDisabled.setPrimaryHeader(CCSDS::PrimaryHeader{
        0, 0, 0, 0x123, CCSDS::UNSEGMENTED, 7, 0
      }); !result) {
    return failResult("CRC-disabled primary header", result.error());
  }
  if (const auto result = crcDisabled.setApplicationData({0xAA, 0x55}); !result) {
    return failResult("CRC-disabled application data", result.error());
  }
  const auto crcDisabledBytes = crcDisabled.serialize();
  const std::vector<std::uint8_t> expectedCrcDisabled{
    0x01, 0x23, 0xC0, 0x07, 0x00, 0x01, 0xAA, 0x55
  };
  if (crcDisabledBytes != expectedCrcDisabled) {
    return fail("CRC-disabled vector", "packet bytes differ from the expected vector");
  }

  CCSDS::Packet decodedCrcDisabled;
  decodedCrcDisabled.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
  const auto crcDisabledConsumed = decodedCrcDisabled.deserializeBounded(crcDisabledBytes);
  if (!crcDisabledConsumed || crcDisabledConsumed.value() != crcDisabledBytes.size()
      || decodedCrcDisabled.getPrimaryHeader().getSequenceCount() != 7U
      || decodedCrcDisabled.getApplicationDataBytes()
         != std::vector<std::uint8_t>({0xAA, 0x55})
      || decodedCrcDisabled.getCRC() != 0U) {
    return fail("CRC-disabled decode", "CRC-free packet did not round-trip");
  }

  std::cout << "CCSDSPack installed shared-library consumer passed.\n";
  return 0;
}
