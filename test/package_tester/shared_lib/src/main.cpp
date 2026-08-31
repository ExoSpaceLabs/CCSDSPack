// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSPack.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class CustomSecondaryHeader final : public ccsds::SecondaryHeaderAbstract {
public:
  CustomSecondaryHeader() { variableLength = true; }

  explicit CustomSecondaryHeader(const std::vector<std::uint8_t> &data) : m_data(data) {
    variableLength = true;
  }

  [[nodiscard]] ccsds::ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
    m_data = data;
    return true;
  }

  [[nodiscard]] std::uint16_t getSize() const override {
    return static_cast<std::uint16_t>(m_data.size());
  }

  [[nodiscard]] std::string getType() const override { return "CustomSecondaryHeader"; }
  [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }

  void update(ccsds::DataField *) override {
    m_dataLength = static_cast<std::uint16_t>(m_data.size());
  }

  ccsds::ResultBool loadFromConfig(const ccsds::Config &) override { return true; }

private:
  std::vector<std::uint8_t> m_data{};
  std::uint16_t m_dataLength{};
};

namespace {
  int fail(const std::string &stage, const std::string &message) {
    std::cerr << "[external consumer] " << stage << ": " << message << '\n';
    return 1;
  }

  int failResult(const std::string &stage, const ccsds::Error &error) {
    std::cerr << "[external consumer] " << stage << ": " << error.message()
              << " (code " << error.code() << ")\n";
    return error.code() == 0 ? 1 : error.code();
  }
}

int main() {
  ccsds::Packet templatePacket;
  if (const auto result = templatePacket.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 1, 0, 0x123, ccsds::UNSEGMENTED, 5, 0
      }); !result) {
    return failResult("set primary header", result.error());
  }

  if (const auto result = templatePacket.RegisterSecondaryHeader<CustomSecondaryHeader>(); !result) {
    return failResult("register custom secondary header", result.error());
  }

  const std::vector<std::uint8_t> secondaryHeader{0x77, 0xFA, 0x0B, 0x00, 0x00, 0x0B, 0x05};
  if (const auto result = templatePacket.setSecondaryHeader(
        secondaryHeader, "CustomSecondaryHeader"); !result) {
    return failResult("set custom secondary header", result.error());
  }

  ccsds::Manager manager;
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

  const auto packetsResult = manager.getPacketsBuffer();
  if (!packetsResult) return failResult("serialize Manager packets", packetsResult.error());
  const auto &packetsData = packetsResult.value();
  if (packetsData != expectedPacket) {
    return fail("wire vector", "generated generic packet bytes differ from the expected vector");
  }
  if (manager.getSequenceCount() != 6U || manager.getTotalPackets() != 1U) {
    return fail("automatic sequence count", "Manager did not consume exactly one sequence count");
  }

  const auto templateBytesResult = manager.getPacketTemplate();
  if (!templateBytesResult || templateBytesResult.value().empty()) {
    return templateBytesResult
      ? fail("getPacketTemplate", "serialized template is empty")
      : failResult("getPacketTemplate", templateBytesResult.error());
  }

  const auto indexedPacketResult = manager.getPacketBufferAtIndex(0);
  if (!indexedPacketResult) return failResult("getPacketBufferAtIndex", indexedPacketResult.error());
  if (indexedPacketResult.value() != expectedPacket) {
    return fail("getPacketBufferAtIndex", "indexed packet differs from stream bytes");
  }

  ccsds::Packet decoded;
  if (const auto result = decoded.RegisterSecondaryHeader<CustomSecondaryHeader>(); !result) {
    return failResult("register decoder secondary header", result.error());
  }
  // Preserve the custom secondary-header schema as well as its explicit variable
  // byte boundary. The opaque-size overload intentionally creates BufferHeader.
  const auto consumed = decoded.deserializeBounded(
    packetsData, "CustomSecondaryHeader", static_cast<std::int32_t>(secondaryHeader.size()));
  if (!consumed) return failResult("bounded custom decode", consumed.error());
  if (consumed.value() != packetsData.size()) {
    return fail("bounded custom decode", "consumed-byte count differs from packet size");
  }

  const ccsds::Packet &view = decoded;
  const auto &header = view.getPrimaryHeader();
  if (header.getVersionNumber() != 0U
      || header.getType() != 1U
      || header.getSecondaryHeaderFlag() != 1U
      || header.getAPID() != 0x123U
      || header.getSequenceFlags() != ccsds::UNSEGMENTED
      || header.getSequenceCount() != 5U
      || view.getSecondaryHeaderBytes() != secondaryHeader
      || !view.getSecondaryHeader()
      || view.getSecondaryHeader()->getType() != "CustomSecondaryHeader"
      || view.getApplicationDataBytes() != std::vector<std::uint8_t>({0x01, 0x02, 0x03})
      || view.getCRC() != 0xB745U) {
    return fail("decoded fields", "decoded packet does not match the expected logical fields");
  }

  if (decoded.getPrimaryHeader64bit() == 0U
      || decoded.getFullPacketLength() != expectedPacket.size()
      || decoded.getSerializedSize() != expectedPacket.size()
      || !decoded.getSecondaryHeaderFlag()
      || decoded.getCRCVectorBytes() != std::vector<std::uint8_t>({0xB7, 0x45})) {
    return fail("packet API", "packet getters returned unexpected values");
  }
  (void)decoded.getDataField();
  (void)decoded.getPrimaryHeader();

  ccsds::Validator validator(templatePacket);
  validator.configure(true, false, true);
  const auto validation = validator.validate(decoded);
  if (!validation.valid()
      || !validation.passed(ccsds::ValidationCode::TemplateSecondaryHeader)) {
    return fail("Validator", "valid custom packet was rejected against its template contract");
  }

  ccsds::Packet mismatchedIdentifier = decoded;
  if (const auto result = mismatchedIdentifier.getPrimaryHeader().setType(0U); !result) {
    return failResult("prepare mismatched identifier", result.error());
  }
  if (manager.addPacket(mismatchedIdentifier)) {
    return fail("Manager identifier", "packet with a different identifier was accepted");
  }

  ccsds::Packet crcDisabled;
  crcDisabled.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
  if (const auto result = crcDisabled.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 0, 0, 0x123, ccsds::UNSEGMENTED, 7, 0
      }); !result) {
    return failResult("CRC-disabled primary header", result.error());
  }
  if (const auto result = crcDisabled.setApplicationData({0xAA, 0x55}); !result) {
    return failResult("CRC-disabled application data", result.error());
  }
  const auto crcDisabledResult = crcDisabled.serialize();
  if (!crcDisabledResult) return failResult("serialize CRC-disabled packet", crcDisabledResult.error());
  const auto &crcDisabledBytes = crcDisabledResult.value();
  const std::vector<std::uint8_t> expectedCrcDisabled{
    0x01, 0x23, 0xC0, 0x07, 0x00, 0x01, 0xAA, 0x55
  };
  if (crcDisabledBytes != expectedCrcDisabled) {
    return fail("CRC-disabled vector", "packet bytes differ from the expected vector");
  }

  ccsds::Packet decodedCrcDisabled;
  decodedCrcDisabled.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
  const auto crcDisabledConsumed = decodedCrcDisabled.deserializeBounded(crcDisabledBytes);
  if (!crcDisabledConsumed || crcDisabledConsumed.value() != crcDisabledBytes.size()
      || decodedCrcDisabled.getPrimaryHeader().getSequenceCount() != 7U
      || decodedCrcDisabled.getApplicationDataBytes()
         != std::vector<std::uint8_t>({0xAA, 0x55})
      || decodedCrcDisabled.getCRC() != 0U) {
    return fail("CRC-disabled decode", "CRC-free packet did not round-trip");
  }

  // Installed-package coverage for the final profile-free PUS API.
  ccsds::Packet pusSource;
  if (const auto result = pusSource.setPrimaryHeader(ccsds::PrimaryHeader{
        0U, 0U, 0U, 0x42U, ccsds::UNSEGMENTED, 0U, 0U}); !result) {
    return failResult("PUS primary header", result.error());
  }
  if (const auto result = pusSource.setSecondaryHeader(
        std::make_shared<ccsds::pus::rev_c::TcHeader>(
          17U, 1U, 0x1234U, 0x09U)); !result) {
    return failResult("PUS secondary header", result.error());
  }
  if (pusSource.getDirection() != ccsds::PacketDirection::Telecommand) {
    return fail("PUS direction", "TcHeader did not synchronize Packet direction");
  }
  if (const auto result = pusSource.setApplicationData({0xDEU, 0xADU}); !result) {
    return failResult("PUS application data", result.error());
  }
  const auto pusWire = pusSource.serialize();
  if (!pusWire) return failResult("PUS serialize", pusWire.error());

  ccsds::Packet pusDecoded;
  const auto pusConsumed = pusDecoded.deserializeBounded<ccsds::pus::rev_c::TcHeader>(
    pusWire.value());
  if (!pusConsumed || pusConsumed.value() != pusWire.value().size()) {
    return pusConsumed
      ? fail("typed PUS decode", "consumed-byte count differs from PUS packet size")
      : failResult("typed PUS decode", pusConsumed.error());
  }
  const auto pusHeader = std::static_pointer_cast<const ccsds::pus::rev_c::TcHeader>(
    pusDecoded.getSecondaryHeader());
  if (!pusHeader || pusHeader->getRevision() != ccsds::pus::Revision::C
      || pusHeader->getDirection() != ccsds::PacketDirection::Telecommand
      || pusHeader->getSourceId() != 0x1234U
      || pusDecoded.getApplicationDataBytes() != std::vector<std::uint8_t>({0xDEU, 0xADU})) {
    return fail("typed PUS decode", "profile-free PUS packet did not round-trip");
  }
  const auto pusReport = ccsds::Validator{}.validate(pusDecoded);
  if (!pusReport.valid() || !pusReport.passed(ccsds::ValidationCode::PusTailoring)) {
    return fail("PUS Validator", "installed PUS public surface failed validation");
  }

  ccsds::Packet invalidVersion;
  if (const auto result = invalidVersion.setPrimaryHeader(ccsds::PrimaryHeader{
        1, 0, 0, 1, ccsds::UNSEGMENTED, 0, 0
      }); !result) {
    return failResult("non-zero PVN setup", result.error());
  }
  if (const auto result = invalidVersion.setApplicationData({0x01}); !result) {
    return failResult("non-zero PVN data", result.error());
  }
  if (invalidVersion.serialize()) {
    return fail("Packet Version Number", "non-zero PVN was serialized as a Space Packet");
  }

  ccsds::Packet invalidIdle;
  if (const auto result = invalidIdle.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 0, 0, ccsds::IDLE_APID, ccsds::UNSEGMENTED, 0, 0
      }); !result) {
    return failResult("Idle Packet setup", result.error());
  }
  if (const auto result = invalidIdle.setSecondaryHeader({0x01}); !result) {
    return failResult("Idle Packet secondary header", result.error());
  }
  if (const auto result = invalidIdle.setApplicationData({0x00}); !result) {
    return failResult("Idle Packet data", result.error());
  }
  if (invalidIdle.serialize()) {
    return fail("Idle Packet", "Idle Packet with a secondary header was serialized");
  }

  ccsds::Packet validIdle;
  if (const auto result = validIdle.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 0, 0, ccsds::IDLE_APID, ccsds::UNSEGMENTED, 0, 0
      }); !result) {
    return failResult("valid Idle Packet setup", result.error());
  }
  if (const auto result = validIdle.setApplicationData({0x00}); !result) {
    return failResult("valid Idle Packet data", result.error());
  }
  const auto validIdleResult = validIdle.serialize();
  if (!validIdleResult) return failResult("serialize valid Idle Packet", validIdleResult.error());
  if (validIdle.getSerializedSize() != validIdleResult.value().size()) {
    return fail("valid Idle Packet", "conformant Idle Packet did not serialize");
  }

  std::cout << "CCSDSPack installed shared-library consumer passed.\n";
  return 0;
}
