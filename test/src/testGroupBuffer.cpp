// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "tests.h"
#include <CCSDSPack.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

void testGroupBuffer(TestManager *tester, const std::string &description) {
  std::cout << "\n" << description << '\n';

  tester->unitTest("Declared packet size requires only the six-byte primary header", []() {
    const std::uint8_t header[6]{0x01, 0x23, 0xC0, 0x07, 0x00, 0x03};
    const auto result = ccsds::buffer::declaredPacketSize(header, sizeof(header));
    return result && result.value() == 10U;
  });

  tester->unitTest("Declared packet size rejects null, short, and unsupported-version input", []() {
    const auto nullResult = ccsds::buffer::declaredPacketSize(nullptr, 6U);
    const std::uint8_t shortHeader[5]{};
    const auto shortResult = ccsds::buffer::declaredPacketSize(shortHeader, sizeof(shortHeader));
    const std::uint8_t wrongVersion[6]{0x20, 0, 0, 0, 0, 0};
    const auto versionResult = ccsds::buffer::declaredPacketSize(wrongVersion, sizeof(wrongVersion));
    return !nullResult && nullResult.error().code() == ccsds::ErrorCode::NULL_POINTER
           && !shortResult && shortResult.error().code() == ccsds::ErrorCode::INVALID_HEADER_DATA
           && !versionResult && versionResult.error().code() == ccsds::ErrorCode::INVALID_HEADER_DATA;
  });

  tester->unitTest("Raw generic Packet bounded parsing matches vector parsing", []() {
    ccsds::Packet outgoing;
    outgoing.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    TEST_VOID(outgoing.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 0, 0, 0x321, ccsds::UNSEGMENTED, 9, 0
    }));
    const std::vector<std::uint8_t> payload{0x10, 0x20, 0x30, 0x40};
    TEST_VOID(outgoing.setApplicationData(payload));

    std::vector<std::uint8_t> wire;
    TEST_RET(wire, outgoing.serialize());

    ccsds::Packet incoming;
    incoming.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    std::size_t consumed{};
    TEST_RET(consumed, ccsds::buffer::deserializeBounded(
      incoming, wire.data(), wire.size()));

    return consumed == wire.size()
           && incoming.getPrimaryHeader().getAPID() == 0x321U
           && incoming.getPrimaryHeader().getSequenceCount() == 9U
           && incoming.getApplicationDataBytes() == payload;
  });

  tester->unitTest("Raw PUS-C Packet parsing uses the active profile and selector", []() {
    const auto profile = ccsds::pus::makeProfile(
      ccsds::pus::Revision::C, ccsds::pus::Direction::Telecommand);

    ccsds::Packet outgoing;
    TEST_VOID(outgoing.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 1, 1, 0x42, ccsds::UNSEGMENTED, 3, 0
    }));
    TEST_VOID(outgoing.setMissionProfile(profile));
    TEST_VOID(outgoing.setSecondaryHeader(
      std::make_shared<ccsds::pus::rev_c::TcHeader>(
        profile, 17U, 1U, 0x1234U, 0x09U)));
    TEST_VOID(outgoing.setApplicationData({0xAA, 0x55}));

    std::vector<std::uint8_t> wire;
    TEST_RET(wire, outgoing.serialize());

    ccsds::Packet incoming;
    TEST_VOID(incoming.setMissionProfile(profile));
    std::size_t consumed{};
    TEST_RET(consumed, ccsds::buffer::deserializeBounded(
      incoming, wire.data(), wire.size(),
      ccsds::pus::selector(profile.pusRevision, profile.direction)));

    const auto secondary = incoming.getSecondaryHeader();
    return consumed == wire.size()
           && secondary && secondary->isPusHeader()
           && incoming.getApplicationDataBytes() == std::vector<std::uint8_t>({0xAA, 0x55});
  });

  tester->unitTest("Manager accepts raw application and packet-stream buffers", []() {
    ccsds::Packet packetTemplate;
    packetTemplate.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    TEST_VOID(packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 0, 0, 0x155, ccsds::UNSEGMENTED, 0, 0
    }));
    packetTemplate.setDataFieldSize(32U);

    ccsds::Manager sender;
    TEST_VOID(sender.setPacketTemplate(packetTemplate));
    sender.setAutoValidateEnable(false);
    const std::uint8_t payload[]{0xA0, 0xA1, 0xA2, 0xA3};
    TEST_VOID(sender.setApplicationData(payload, sizeof(payload)));

    std::vector<std::uint8_t> stream;
    TEST_RET(stream, sender.getPacketsBuffer());

    ccsds::Manager receiver;
    TEST_VOID(receiver.setPacketTemplate(packetTemplate));
    receiver.setAutoValidateEnable(false);
    TEST_VOID(receiver.load(stream.data(), stream.size()));

    std::vector<std::uint8_t> rebuilt;
    TEST_RET(rebuilt, receiver.getApplicationDataBuffer());
    return rebuilt == std::vector<std::uint8_t>(payload, payload + sizeof(payload));
  });

  tester->unitTest("Const Manager reference accessors avoid packet/template copies", []() {
    ccsds::Packet packetTemplate;
    packetTemplate.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    TEST_VOID(packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 0, 0, 0x155, ccsds::UNSEGMENTED, 0, 0
    }));
    packetTemplate.setDataFieldSize(32U);

    ccsds::Manager manager;
    TEST_VOID(manager.setPacketTemplate(packetTemplate));
    manager.setAutoValidateEnable(false);
    const std::uint8_t payload[]{0x01, 0x02};
    TEST_VOID(manager.setApplicationData(payload, sizeof(payload)));

    const ccsds::Manager &view = manager;
    const auto &templateRef = view.getTemplateReference();
    const auto &packetsRef = view.getPacketsReference();
    const auto &validatorRef = view.getValidatorReference();
    (void)validatorRef;

    return templateRef.getPrimaryHeader().getAPID() == 0x155U
           && packetsRef.size() == 1U
           && packetsRef.front().getApplicationDataBytes()
              == std::vector<std::uint8_t>({0x01, 0x02});
  });

  tester->unitTest("Error codes expose stable symbolic names", []() {
    struct Entry { ccsds::ErrorCode code; const char *name; };
    const std::array<Entry, 14> entries{{
      {ccsds::ErrorCode::NONE, "NONE"},
      {ccsds::ErrorCode::UNKNOWN_ERROR, "UNKNOWN_ERROR"},
      {ccsds::ErrorCode::NO_DATA, "NO_DATA"},
      {ccsds::ErrorCode::INVALID_DATA, "INVALID_DATA"},
      {ccsds::ErrorCode::INVALID_HEADER_DATA, "INVALID_HEADER_DATA"},
      {ccsds::ErrorCode::INVALID_SECONDARY_HEADER_DATA, "INVALID_SECONDARY_HEADER_DATA"},
      {ccsds::ErrorCode::INVALID_APPLICATION_DATA, "INVALID_APPLICATION_DATA"},
      {ccsds::ErrorCode::NULL_POINTER, "NULL_POINTER"},
      {ccsds::ErrorCode::INVALID_CHECKSUM, "INVALID_CHECKSUM"},
      {ccsds::ErrorCode::VALIDATION_FAILURE, "VALIDATION_FAILURE"},
      {ccsds::ErrorCode::TEMPLATE_SET_FAILURE, "TEMPLATE_SET_FAILURE"},
      {ccsds::ErrorCode::FILE_READ_ERROR, "FILE_READ_ERROR"},
      {ccsds::ErrorCode::FILE_WRITE_ERROR, "FILE_WRITE_ERROR"},
      {ccsds::ErrorCode::CONFIG_FILE_ERROR, "CONFIG_FILE_ERROR"}
    }};

    for (const auto &entry : entries) {
      if (std::string(ccsds::errorCodeName(entry.code)) != entry.name) return false;
    }
    return std::string(ccsds::errorCodeName(static_cast<ccsds::ErrorCode>(0xFFU)))
           == "UNRECOGNIZED_ERROR_CODE";
  });
}
