// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "tests.h"
#include <CCSDSValidator.h>
#include <PusSecondaryHeaders.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

namespace {
  ccsds::Packet basicPacket(const ccsds::PacketDirection direction) {
    ccsds::Packet packet;
    packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    if (!packet.setPrimaryHeader(ccsds::PrimaryHeader{
          0U, ccsds::packetTypeForDirection(direction), 0U, 42U,
          ccsds::UNSEGMENTED, 0U, 0U})) return {};
    if (!packet.setApplicationData({0xAAU})) return {};
    if (!packet.serialize()) return {};
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  ccsds::Packet pusCTcPacket() {
    ccsds::Packet packet;
    packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    if (!packet.setPrimaryHeader(ccsds::PrimaryHeader{
          0U, 1U, 0U, 42U, ccsds::UNSEGMENTED, 0U, 0U})) return {};
    if (!packet.setSecondaryHeader(
          std::make_shared<ccsds::pus::rev_c::TcHeader>(17U, 1U, 0x1234U, 0x09U))) return {};
    if (!packet.setApplicationData({0xAAU})) return {};
    if (!packet.serialize()) return {};
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  class InvalidRevisionTcHeader final : public ccsds::pus::TcSecondaryHeader {
  public:
    InvalidRevisionTcHeader()
      : TcSecondaryHeader(2U, 0U, 17U, 1U, 0x1234U, 0x09U) {}

    [[nodiscard]] ccsds::pus::Revision getRevision() const noexcept override {
      return ccsds::pus::Revision::Unspecified;
    }
    [[nodiscard]] ccsds::ResultBool deserialize(
        const std::vector<std::uint8_t> &data) override {
      return parseTcBody(data, 1U);
    }
    [[nodiscard]] std::uint16_t getSize() const override { return tcSize(); }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override {
      std::vector<std::uint8_t> bytes{0x20U | 0x09U};
      appendTcBody(bytes);
      return bytes;
    }
  };

  class InvalidDirectionPusHeader final : public ccsds::pus::SecondaryHeader {
  public:
    InvalidDirectionPusHeader() : SecondaryHeader(0U) {}

    [[nodiscard]] ccsds::pus::Revision getRevision() const noexcept override {
      return ccsds::pus::Revision::C;
    }
    [[nodiscard]] ccsds::PacketDirection getDirection() const noexcept override {
      return static_cast<ccsds::PacketDirection>(0x7FU);
    }
    [[nodiscard]] ccsds::ResultBool deserialize(
        const std::vector<std::uint8_t> &data) override {
      return data.size() == 1U
               ? ccsds::ResultBool{true}
               : ccsds::ResultBool{ccsds::Error{
                   ccsds::ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Invalid-direction PUS evidence header requires one byte."}};
    }
    [[nodiscard]] std::uint16_t getSize() const override { return 1U; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return {0x20U}; }
  };
}

void testGroupEvidence(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupEvidence: " << description << std::endl;

  tester->unitTest("PUS-C TC acknowledgement flags match the independent 16-value ECSS byte matrix.", [] {
    struct AckVector {
      std::uint8_t flags;
      std::array<std::uint8_t, 5U> bytes;
    };

    constexpr std::array<AckVector, 16U> vectors{{
      {0x0U, {0x20U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0x1U, {0x21U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0x2U, {0x22U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0x3U, {0x23U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0x4U, {0x24U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0x5U, {0x25U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0x6U, {0x26U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0x7U, {0x27U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0x8U, {0x28U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0x9U, {0x29U, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0xAU, {0x2AU, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0xBU, {0x2BU, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0xCU, {0x2CU, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0xDU, {0x2DU, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0xEU, {0x2EU, 0x11U, 0x01U, 0x12U, 0x34U}},
      {0xFU, {0x2FU, 0x11U, 0x01U, 0x12U, 0x34U}}
    }};

    for (const auto &entry : vectors) {
      ccsds::pus::rev_c::TcHeader encoded(17U, 1U, 0x1234U, entry.flags);
      const std::vector<std::uint8_t> expected(entry.bytes.begin(), entry.bytes.end());
      if (encoded.serialize() != expected) return false;

      ccsds::pus::rev_c::TcHeader decoded;
      if (!decoded.deserialize(expected)) return false;
      if (decoded.getAcknowledgementFlags() != entry.flags
          || decoded.getServiceType() != 17U
          || decoded.getServiceSubtype() != 1U
          || decoded.getSourceId() != 0x1234U) return false;
    }
    return true;
  });

  tester->unitTest("Validator directly reports an invalid primary-header state.", [] {
    auto packet = basicPacket(ccsds::PacketDirection::Telemetry);
    if (packet.getPrimaryHeader().setAPID(0x0800U)) return false;
    ccsds::Validator validator;
    const auto report = validator.validate(packet);
    return !report.valid() && report.failed(ccsds::ValidationCode::PrimaryHeader);
  });

  tester->unitTest("Validator directly reports an unsupported Space Packet version.", [] {
    auto packet = basicPacket(ccsds::PacketDirection::Telemetry);
    if (!packet.getPrimaryHeader().setVersionNumber(1U)) return false;
    ccsds::Validator validator;
    const auto report = validator.validate(packet);
    return report.passed(ccsds::ValidationCode::PrimaryHeader)
           && report.failed(ccsds::ValidationCode::PacketVersion);
  });

  tester->unitTest("Validator directly reports secondary-header flag/object mismatch.", [] {
    auto packet = pusCTcPacket();
    if (!packet.getPrimaryHeader().setSecondaryHeaderFlag(0U)) return false;
    ccsds::Validator validator;
    return validator.validate(packet).failed(ccsds::ValidationCode::SecondaryHeaderPresence);
  });

  tester->unitTest("Validator directly reports invalid PUS revision, direction, and tailoring states.", [] {
    ccsds::Packet badRevision;
    badRevision.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    if (!badRevision.setPrimaryHeader(ccsds::PrimaryHeader{
          0U, 1U, 0U, 42U, ccsds::UNSEGMENTED, 0U, 0U})
        || !badRevision.setSecondaryHeader(std::make_shared<InvalidRevisionTcHeader>())
        || !badRevision.setApplicationData({0xAAU})) return false;
    ccsds::Validator validator;
    const auto revisionReport = validator.validate(badRevision);
    if (!revisionReport.failed(ccsds::ValidationCode::PusRevision)
        || !revisionReport.failed(ccsds::ValidationCode::PusTailoring)) return false;

    ccsds::Packet badDirection;
    badDirection.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    if (!badDirection.setPrimaryHeader(ccsds::PrimaryHeader{
          0U, 0U, 0U, 42U, ccsds::UNSEGMENTED, 0U, 0U})
        || !badDirection.setSecondaryHeader(std::make_shared<InvalidDirectionPusHeader>())
        || !badDirection.setApplicationData({0xAAU})) return false;
    const auto directionReport = validator.validate(badDirection);
    if (!directionReport.failed(ccsds::ValidationCode::PusDirection)
        || !directionReport.failed(ccsds::ValidationCode::PusPacketType)
        || !directionReport.failed(ccsds::ValidationCode::PusTailoring)) return false;

    ccsds::pus::rev_a::TcTailoring tailoring;
    tailoring.sourceIdOctets = 3U;
    ccsds::Packet badTailoring;
    badTailoring.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    if (!badTailoring.setPrimaryHeader(ccsds::PrimaryHeader{
          0U, 1U, 0U, 42U, ccsds::UNSEGMENTED, 0U, 0U})
        || !badTailoring.setSecondaryHeader(
          std::make_shared<ccsds::pus::rev_a::TcHeader>(tailoring, 17U, 1U, 0U, 0U))
        || !badTailoring.setApplicationData({0xAAU})) return false;
    return validator.validate(badTailoring).failed(ccsds::ValidationCode::PusTailoring);
  });

  tester->unitTest("Validator directly reports overflowing TM destination ID and time-reference status.", [] {
    ccsds::pus::rev_a::TmTailoring aTailoring;
    aTailoring.destinationIdOctets = 1U;
    ccsds::Packet badDestination;
    badDestination.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    if (!badDestination.setPrimaryHeader(ccsds::PrimaryHeader{
          0U, 0U, 0U, 42U, ccsds::UNSEGMENTED, 0U, 0U})
        || !badDestination.setSecondaryHeader(
          std::make_shared<ccsds::pus::rev_a::TmHeader>(aTailoring, 3U, 25U, 0U, 0x1FFU))
        || !badDestination.setApplicationData({0xAAU})) return false;
    ccsds::Validator validator;
    if (!validator.validate(badDestination).failed(ccsds::ValidationCode::PusDestinationId))
      return false;

    ccsds::Packet badStatus;
    badStatus.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    if (!badStatus.setPrimaryHeader(ccsds::PrimaryHeader{
          0U, 0U, 0U, 42U, ccsds::UNSEGMENTED, 0U, 0U})
        || !badStatus.setSecondaryHeader(
          std::make_shared<ccsds::pus::rev_c::TmHeader>(3U, 25U, 1U, 0x1234U, 0x10U))
        || !badStatus.setApplicationData({0xAAU})) return false;
    return validator.validate(badStatus).failed(ccsds::ValidationCode::PusTimeReferenceStatus);
  });

  tester->unitTest("Every public ValidationCode has a stable symbolic name and report capacity.", [] {
    constexpr std::array<ccsds::ValidationCode, 26U> codes{{
      ccsds::ValidationCode::PrimaryHeader,
      ccsds::ValidationCode::PacketVersion,
      ccsds::ValidationCode::PacketDataLength,
      ccsds::ValidationCode::Crc16,
      ccsds::ValidationCode::SecondaryHeaderPresence,
      ccsds::ValidationCode::SecondaryHeaderDirection,
      ccsds::ValidationCode::SequenceFlags,
      ccsds::ValidationCode::SequenceCount,
      ccsds::ValidationCode::PacketIdentifier,
      ccsds::ValidationCode::SegmentationClass,
      ccsds::ValidationCode::TemplatePacketErrorControl,
      ccsds::ValidationCode::TemplateSecondaryHeader,
      ccsds::ValidationCode::PusHeader,
      ccsds::ValidationCode::PusRevision,
      ccsds::ValidationCode::PusDirection,
      ccsds::ValidationCode::PusPacketType,
      ccsds::ValidationCode::PusTailoring,
      ccsds::ValidationCode::PusSecondaryHeaderSize,
      ccsds::ValidationCode::PusReservedBits,
      ccsds::ValidationCode::PusSpareFields,
      ccsds::ValidationCode::PusAcknowledgement,
      ccsds::ValidationCode::PusSourceId,
      ccsds::ValidationCode::PusDestinationId,
      ccsds::ValidationCode::PusPacketSubcounter,
      ccsds::ValidationCode::PusTimeReferenceStatus,
      ccsds::ValidationCode::PusTimestamp
    }};
    if (ccsds::ValidationReport::Capacity < codes.size()) return false;
    for (const auto code : codes) {
      const auto *name = ccsds::validationCodeName(code);
      if (!name || std::strcmp(name, "Unknown validation check") == 0) return false;
    }
    return true;
  });
}
