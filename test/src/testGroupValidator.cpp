// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSValidator.h>
#include <PusSecondaryHeaders.h>
#include <iostream>
#include <memory>
#include <vector>
#include "tests.h"

namespace {
  ccsds::Packet finalizedPacket(
      const std::uint16_t apid,
      const ccsds::ESequenceFlag flags,
      const std::uint16_t count,
      const std::vector<std::uint8_t> &data = {1, 2, 3},
      const ccsds::PacketErrorControlMode mode = ccsds::PacketErrorControlMode::CRC16,
      const std::uint8_t version = 0U,
      const std::uint8_t type = 0U,
      const std::uint8_t secondaryHeaderFlag = 0U) {
    ccsds::Packet packet;
    packet.setPacketErrorControlMode(mode);
    const auto headerResult = packet.setPrimaryHeader(
      ccsds::PrimaryHeader{version, type, secondaryHeaderFlag, apid, flags, count, 0});
    if (!headerResult) return {};
    const auto dataResult = packet.setApplicationData(data);
    if (!dataResult || !packet.serialize()) return {};
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  ccsds::Packet rawPacketWithoutCRC(const std::uint16_t apid,
                                    const ccsds::ESequenceFlag flags,
                                    const std::uint16_t count,
                                    const std::uint16_t dataLength,
                                    const std::vector<std::uint8_t> &data = {1, 2, 3}) {
    ccsds::Packet packet;
    packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    if (!packet.setPrimaryHeader(
          ccsds::PrimaryHeader{0, 0, 0, apid, flags, count, dataLength})
        || !packet.setApplicationData(data)) {
      return {};
    }
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  ccsds::Validator validatorWithTemplate(const ccsds::Packet &packet,
                                          const bool coherence = true,
                                          const bool sequence = true) {
    ccsds::Validator validator;
    validator.configure(coherence, sequence, true);
    validator.setTemplatePacket(packet);
    return validator;
  }

  ccsds::Packet pusCTcPacket(ccsds::MissionProfile profile) {
    ccsds::Packet packet;
    if (!packet.setPrimaryHeader({0U, 1U, 1U, 42U, ccsds::UNSEGMENTED, 0U, 0U})) return {};
    if (!packet.setMissionProfile(profile)) return {};
    if (!packet.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TcHeader>(
          profile, 17U, 1U, 0x1234U, 0x09U))) return {};
    if (!packet.setApplicationData({0xAAU})) return {};
    if (!packet.serialize()) return {};
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  class MalformedPusCTcHeader final : public ccsds::pus::TcSecondaryHeader {
  public:
    explicit MalformedPusCTcHeader(ccsds::MissionProfile profile)
      : TcSecondaryHeader(profile, 17U, 1U, 0x1234U, 0x09U) {}

    [[nodiscard]] ccsds::ResultBool deserialize(
        const std::vector<std::uint8_t> &data) override {
      (void)data;
      return true;
    }

    [[nodiscard]] std::uint16_t getSize() const override { return tcSize(); }

    [[nodiscard]] std::vector<std::uint8_t> serialize() const override {
      std::vector<std::uint8_t> bytes{
        0x39U, 17U, 1U, 0x12U, 0x34U
      };
      bytes.insert(bytes.end(), m_profile.secondaryHeaderSpareOctets, 0xFFU);
      return bytes;
    }
  };
}

void testGroupValidator(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupValidator: " << description << std::endl;

  tester->unitTest("Validator accepts a non-zero unsegmented sequence count.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 5)).valid();
  });

  tester->unitTest("Validator accepts first segment sequence count zero.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, ccsds::FIRST_SEGMENT, 0)).valid();
  });

  tester->unitTest("Validator tracks a coherent segmented sequence.", [] {
    const auto first = finalizedPacket(1, ccsds::FIRST_SEGMENT, 10);
    const auto continuing = finalizedPacket(1, ccsds::CONTINUING_SEGMENT, 11);
    const auto last = finalizedPacket(1, ccsds::LAST_SEGMENT, 12);
    ccsds::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(first).valid()
           && validator.validate(continuing).valid()
           && validator.validate(last).valid();
  });

  tester->unitTest("Validator rejects continuation without an open segmented sequence.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    const auto report = validator.validate(
      finalizedPacket(1, ccsds::CONTINUING_SEGMENT, 7));
    return !report.valid()
           && report.failed(ccsds::ValidationCode::SequenceFlags)
           && report.passed(ccsds::ValidationCode::SequenceCount);
  });

  tester->unitTest("Validator rejects an unsegmented packet before a segmented sequence is closed.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    if (!validator.validate(finalizedPacket(1, ccsds::FIRST_SEGMENT, 3)).valid()) return false;
    const auto report = validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 4));
    return report.failed(ccsds::ValidationCode::SequenceFlags)
           && report.passed(ccsds::ValidationCode::SequenceCount);
  });

  tester->unitTest("Validator rejects a discontinuous sequence count.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    if (!validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 20)).valid()) return false;
    const auto report = validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 22));
    return report.passed(ccsds::ValidationCode::SequenceFlags)
           && report.failed(ccsds::ValidationCode::SequenceCount);
  });

  tester->unitTest("Validator sequence count rolls over modulo 16384.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 0x3FFFU)).valid()
           && validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 0U)).valid();
  });

  tester->unitTest("Validator accepts a packet matching its template identifier.", [] {
    const auto packet = finalizedPacket(0x123, ccsds::UNSEGMENTED, 4,
                                        {1, 2, 3}, ccsds::PacketErrorControlMode::CRC16,
                                        0, 1, 0);
    auto validator = validatorWithTemplate(packet, false, true);
    return validator.validate(packet).valid();
  });

  tester->unitTest("Validator rejects a packet with a different template identifier.", [] {
    const auto templatePacket = finalizedPacket(0x123, ccsds::UNSEGMENTED, 4,
                                                {1, 2, 3}, ccsds::PacketErrorControlMode::CRC16,
                                                0, 1, 0);
    const auto packet = finalizedPacket(0x124, ccsds::UNSEGMENTED, 4,
                                        {1, 2, 3}, ccsds::PacketErrorControlMode::CRC16,
                                        0, 1, 0);
    auto validator = validatorWithTemplate(templatePacket, false, true);
    const auto report = validator.validate(packet);
    return report.failed(ccsds::ValidationCode::PacketIdentifier);
  });

  tester->unitTest("Validator rejects segmented state against an unsegmented template.", [] {
    const auto templatePacket = finalizedPacket(1, ccsds::UNSEGMENTED, 0);
    const auto packet = finalizedPacket(1, ccsds::FIRST_SEGMENT, 1);
    auto validator = validatorWithTemplate(templatePacket, false, true);
    return validator.validate(packet).failed(ccsds::ValidationCode::SegmentationClass);
  });

  tester->unitTest("Validator accepts packets without packet error control.", [] {
    auto packet = finalizedPacket(1, ccsds::UNSEGMENTED, 9, {1, 2, 3},
                                  ccsds::PacketErrorControlMode::None);
    ccsds::Validator validator;
    validator.configure(true, true, false);
    const auto report = validator.validate(packet);
    return report.valid() && !report.contains(ccsds::ValidationCode::Crc16);
  });

  tester->unitTest("Validator report exposes named checks instead of boolean indices.", [] {
    const auto packet = finalizedPacket(1, ccsds::UNSEGMENTED, 9);
    auto validator = validatorWithTemplate(packet);
    const auto report = validator.validate(packet);
    return report.valid()
           && report.contains(ccsds::ValidationCode::PacketDataLength)
           && report.contains(ccsds::ValidationCode::Crc16)
           && report.contains(ccsds::ValidationCode::PacketIdentifier)
           && report.size() <= ccsds::ValidationReport::Capacity;
  });

  tester->unitTest("Validator isolates a Packet Data Length failure.", [] {
    auto packet = rawPacketWithoutCRC(1, ccsds::UNSEGMENTED, 9, 7);
    auto templatePacket = rawPacketWithoutCRC(1, ccsds::UNSEGMENTED, 9, 2);
    auto validator = validatorWithTemplate(templatePacket);
    const auto report = validator.validate(packet);
    return report.failed(ccsds::ValidationCode::PacketDataLength)
           && report.passed(ccsds::ValidationCode::PacketIdentifier);
  });

  tester->unitTest("Validator isolates a CRC failure.", [] {
    auto packet = finalizedPacket(1, ccsds::UNSEGMENTED, 9, {1, 2, 3});
    TEST_VOID(packet.setApplicationData({1, 2, 4}));
    auto templatePacket = finalizedPacket(1, ccsds::UNSEGMENTED, 9, {1, 2, 3});
    auto validator = validatorWithTemplate(templatePacket);
    const auto report = validator.validate(packet);
    return report.failed(ccsds::ValidationCode::Crc16)
           && report.passed(ccsds::ValidationCode::PacketDataLength);
  });

  tester->unitTest("Validator compares mission profiles explicitly.", [] {
    auto templatePacket = rawPacketWithoutCRC(1, ccsds::UNSEGMENTED, 0, 2);
    auto packet = templatePacket;
    ccsds::MissionProfile profile;
    profile.packetErrorControl = ccsds::PacketErrorControlMode::None;
    TEST_VOID(packet.setMissionProfile(profile));
    auto validator = validatorWithTemplate(templatePacket, false, false);
    return validator.validate(packet).failed(
      ccsds::ValidationCode::TemplateMissionProfile);
  });

  tester->unitTest("Validator reports PUS Packet Type and packet-error-control mismatches.", [] {
    auto profile = ccsds::pus::makeProfile(
      ccsds::pus::Revision::C, ccsds::pus::Direction::Telecommand);
    auto packet = pusCTcPacket(profile);
    packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    TEST_VOID(packet.getPrimaryHeader().setType(0U));

    ccsds::Validator validator;
    const auto report = validator.validate(packet);
    return report.failed(ccsds::ValidationCode::PacketErrorControlProfile)
           && report.failed(ccsds::ValidationCode::PusPacketType)
           && report.passed(ccsds::ValidationCode::PusRevision)
           && report.passed(ccsds::ValidationCode::PusDirection);
  });

  tester->unitTest("Validator distinguishes invalid PUS acknowledgement and source-ID fields.", [] {
    auto profile = ccsds::pus::makeProfile(
      ccsds::pus::Revision::C, ccsds::pus::Direction::Telecommand);
    profile.packetErrorControl = ccsds::PacketErrorControlMode::None;

    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader({0U, 1U, 1U, 42U, ccsds::UNSEGMENTED, 0U, 0U}));
    TEST_VOID(packet.setMissionProfile(profile));
    TEST_VOID(packet.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TcHeader>(
      profile, 17U, 1U, 0x10000U, 0x10U)));
    TEST_VOID(packet.setApplicationData({0xAAU}));

    ccsds::Validator validator;
    const auto report = validator.validate(packet);
    return report.failed(ccsds::ValidationCode::PusAcknowledgement)
           && report.failed(ccsds::ValidationCode::PusSourceId)
           && report.failed(ccsds::ValidationCode::PusSecondaryHeaderSize);
  });

  tester->unitTest("Validator distinguishes PUS reserved bits and spare-field failures.", [] {
    auto profile = ccsds::pus::makeProfile(
      ccsds::pus::Revision::C, ccsds::pus::Direction::Telecommand);
    profile.packetErrorControl = ccsds::PacketErrorControlMode::None;
    profile.secondaryHeaderSpareOctets = 1U;

    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader({0U, 1U, 1U, 42U, ccsds::UNSEGMENTED, 0U, 0U}));
    TEST_VOID(packet.setMissionProfile(profile));
    TEST_VOID(packet.setSecondaryHeader(std::make_shared<MalformedPusCTcHeader>(profile)));
    TEST_VOID(packet.setApplicationData({0xAAU}));

    ccsds::Validator validator;
    const auto report = validator.validate(packet);
    return report.failed(ccsds::ValidationCode::PusReservedBits)
           && report.failed(ccsds::ValidationCode::PusSpareFields);
  });

  tester->unitTest("Validator distinguishes disabled PUS-A TM subcounter state.", [] {
    auto profile = ccsds::pus::makeProfile(
      ccsds::pus::Revision::A, ccsds::pus::Direction::Telemetry);
    profile.packetErrorControl = ccsds::PacketErrorControlMode::None;

    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader({0U, 0U, 1U, 42U, ccsds::UNSEGMENTED, 0U, 0U}));
    TEST_VOID(packet.setMissionProfile(profile));
    TEST_VOID(packet.setSecondaryHeader(std::make_shared<ccsds::pus::rev_a::TmHeader>(
      profile, 3U, 25U, 7U, 0U)));
    TEST_VOID(packet.setApplicationData({0xAAU}));

    ccsds::Validator validator;
    return validator.validate(packet).failed(
      ccsds::ValidationCode::PusPacketSubcounter);
  });

  tester->unitTest("Validator distinguishes an overflowing PUS TM CUC timestamp.", [] {
    auto profile = ccsds::pus::makeProfile(
      ccsds::pus::Revision::C, ccsds::pus::Direction::Telemetry);
    profile.packetErrorControl = ccsds::PacketErrorControlMode::None;
    profile.telemetryTimestampPresent = true;
    profile.telemetryTimeCode = ccsds::time::Format::Cuc;
    profile.telemetryCuc = {
      ccsds::time::Epoch::Ccsds1958Tai,
      ccsds::time::PFieldMode::Implicit,
      4U,
      0U
    };

    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader({0U, 0U, 1U, 42U, ccsds::UNSEGMENTED, 0U, 0U}));
    TEST_VOID(packet.setMissionProfile(profile));
    TEST_VOID(packet.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TmHeader>(
      profile, 3U, 25U, 1U, 0x1234U, 0U,
      ccsds::time::CucTime{0x100000000ULL, 0U})));
    TEST_VOID(packet.setApplicationData({0xAAU}));

    ccsds::Validator validator;
    return validator.validate(packet).failed(ccsds::ValidationCode::PusTimestamp);
  });
}
