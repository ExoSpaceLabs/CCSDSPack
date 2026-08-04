// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "PusSecondaryHeaderFactory.h"
#include "tests.h"
#include <CCSDSPacket.h>
#include <array>
#include <iostream>
#include <memory>
#include <vector>

namespace {
  class ReservedCustomHeader final : public CCSDS::SecondaryHeaderAbstract {
  public:
    [[nodiscard]] CCSDS::ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
      m_data = data;
      return true;
    }
    void update(CCSDS::DataField *) override {}
    [[nodiscard]] std::uint16_t getSize() const override { return 1U; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }
    [[nodiscard]] std::string getType() const override { return "PUS:mission:custom"; }
#ifndef CCSDS_MCU
    CCSDS::ResultBool loadFromConfig(const Config &) override { return true; }
#endif
  private:
    std::vector<std::uint8_t> m_data{0U};
  };

  CCSDS::PrimaryHeader primary(const CCSDS::PacketDirection direction) {
    return {0U,
            static_cast<std::uint8_t>(direction == CCSDS::PacketDirection::Telecommand ? 1U : 0U),
            0U, 1U, CCSDS::UNSEGMENTED, 0U, 0U};
  }
}

void testGroupPus(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupPus: " << description << std::endl;

  tester->unitTest("PUS factory resolves every canonical selector to a fresh matching codec.", [] {
    struct SelectorCase {
      const char *selector;
      CCSDS::PusRevision revision;
      CCSDS::PacketDirection direction;
    };

    constexpr std::array<SelectorCase, 4U> cases{{
      {"PUS:revA:TC", CCSDS::PusRevision::A, CCSDS::PacketDirection::Telecommand},
      {"PUS:revA:TM", CCSDS::PusRevision::A, CCSDS::PacketDirection::Telemetry},
      {"PUS:revC:TC", CCSDS::PusRevision::C, CCSDS::PacketDirection::Telecommand},
      {"PUS:revC:TM", CCSDS::PusRevision::C, CCSDS::PacketDirection::Telemetry}
    }};

    CCSDS::PusSecondaryHeaderFactory factory;
    for (const auto &entry : cases) {
      const auto profile = CCSDS::makePusProfile(entry.revision, entry.direction);
      const auto first = factory.create(entry.selector, profile);
      const auto second = factory.create(entry.selector, profile);
      if (!first || !second || first.value() == second.value()) return false;

      const auto header = std::dynamic_pointer_cast<CCSDS::PusSecondaryHeader>(first.value());
      if (!header || header->getType() != entry.selector
          || header->getRevision() != entry.revision
          || header->getDirection() != entry.direction
          || !header->matchesMissionProfile(profile)) return false;
    }

    const auto aTcProfile = CCSDS::makePusProfile(CCSDS::PusRevision::A,
                                                  CCSDS::PacketDirection::Telecommand);
    if (factory.create("PUS:revA:TM", aTcProfile)
        || factory.create("PUS:revB:TC", aTcProfile)
        || factory.typeIsSupported("PUS:reva:TC")) return false;

    return true;
  });

  tester->unitTest("Custom header registration rejects the reserved PUS namespace.", [] {
    CCSDS::SecondaryHeaderFactory custom;
    return !custom.registerType<ReservedCustomHeader>()
           && CCSDS::PusSecondaryHeaderFactory::isPusSelector("PUS:mission:custom")
           && !CCSDS::PusSecondaryHeaderFactory::isPusSelector("MissionHeader");
  });

  tester->unitTest("PUS-A TC matches the ECSS-E-70-41A field layout.", [] {
    auto profile = CCSDS::makePusProfile(CCSDS::PusRevision::A,
                                         CCSDS::PacketDirection::Telecommand);
    profile.sourceIdOctets = 1U;
    profile.secondaryHeaderSpareOctets = 1U;
    CCSDS::PusATcHeader header(profile, 17U, 2U, 0x55U, 0x0AU);
    const std::vector<std::uint8_t> expected{0x1A, 0x11, 0x02, 0x55, 0x00};
    if (header.serialize() != expected) return false;
    CCSDS::PusATcHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getAcknowledgementFlags() == 0x0AU
           && decoded.getServiceType() == 17U && decoded.getServiceSubtype() == 2U
           && decoded.getSourceId() == 0x55U;
  });

  tester->unitTest("PUS-A TM supports its optional subcounter, destination, time, and spare fields.", [] {
    auto profile = CCSDS::makePusProfile(CCSDS::PusRevision::A,
                                         CCSDS::PacketDirection::Telemetry);
    profile.destinationIdOctets = 1U;
    profile.pusATmPacketSubcounterPresent = true;
    profile.telemetryTimestampPresent = true;
    profile.telemetryTimeCode = CCSDS::TimeCodeFormat::Cuc;
    profile.telemetryTimeCodeOctets = 4U;
    profile.secondaryHeaderSpareOctets = 1U;
    CCSDS::PusATmHeader header(profile, 3U, 25U, 0x44U, 0x7EU, {1U, 2U, 3U, 4U});
    const std::vector<std::uint8_t> expected{
      0x10, 0x03, 0x19, 0x44, 0x7E, 0x01, 0x02, 0x03, 0x04, 0x00};
    if (header.serialize() != expected) return false;
    CCSDS::PusATmHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getPacketSubcounter() == 0x44U
           && decoded.getDestinationId() == 0x7EU
           && decoded.getTimestamp() == std::vector<std::uint8_t>({1U, 2U, 3U, 4U});
  });

  tester->unitTest("PUS-C TC matches the ECSS-E-ST-70-41C field layout.", [] {
    const auto profile = CCSDS::makePusProfile(CCSDS::PusRevision::C,
                                               CCSDS::PacketDirection::Telecommand);
    CCSDS::PusCTcHeader header(profile, 17U, 1U, 0x1234U, 0x0FU);
    const std::vector<std::uint8_t> expected{0x2F, 0x11, 0x01, 0x12, 0x34};
    if (header.serialize() != expected) return false;
    CCSDS::PusCTcHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getSourceId() == 0x1234U
           && decoded.getAcknowledgementFlags() == 0x0FU;
  });

  tester->unitTest("PUS-C TM without time matches the fixed counter and destination layout.", [] {
    const auto profile = CCSDS::makePusProfile(CCSDS::PusRevision::C,
                                               CCSDS::PacketDirection::Telemetry);
    CCSDS::PusCTmHeader header(profile, 3U, 25U, 0x1234U, 0xABCDU, 3U);
    const std::vector<std::uint8_t> expected{0x23, 0x03, 0x19, 0x12, 0x34, 0xAB, 0xCD};
    if (header.serialize() != expected) return false;
    CCSDS::PusCTmHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getMessageTypeCounter() == 0x1234U
           && decoded.getDestinationId() == 0xABCDU
           && decoded.getTimeReferenceStatus() == 3U;
  });

  tester->unitTest("PUS-C TM timestamp and spare sizes are profile-derived.", [] {
    auto profile = CCSDS::makePusProfile(CCSDS::PusRevision::C,
                                         CCSDS::PacketDirection::Telemetry);
    profile.telemetryTimestampPresent = true;
    profile.telemetryTimeCode = CCSDS::TimeCodeFormat::Cuc;
    profile.telemetryTimeCodeOctets = 4U;
    profile.secondaryHeaderSpareOctets = 1U;
    CCSDS::PusCTmHeader header(profile, 5U, 1U, 7U, 0x0102U, 5U,
                              {0x11U, 0x22U, 0x33U, 0x44U});
    const std::vector<std::uint8_t> expected{
      0x25, 0x05, 0x01, 0x00, 0x07, 0x01, 0x02, 0x11, 0x22, 0x33, 0x44, 0x00};
    if (header.getSize() != expected.size() || header.serialize() != expected) return false;
    CCSDS::PusCTmHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getTimestamp() == std::vector<std::uint8_t>({0x11, 0x22, 0x33, 0x44});
  });

  tester->unitTest("Canonical PUS selector round-trips a packet and respects its boundary.", [] {
    const auto profile = CCSDS::makePusProfile(CCSDS::PusRevision::C,
                                               CCSDS::PacketDirection::Telecommand);
    CCSDS::Packet source;
    TEST_VOID(source.setPrimaryHeader(primary(CCSDS::PacketDirection::Telecommand)));
    TEST_VOID(source.setMissionProfile(profile));
    TEST_VOID(source.setSecondaryHeader(
      std::make_shared<CCSDS::PusCTcHeader>(profile, 17U, 1U, 0x1234U, 0x09U)));
    TEST_VOID(source.setApplicationData({0x60U, 0x70U}));
    const auto packetBytes = source.serialize();
    auto stream = packetBytes;
    stream.insert(stream.end(), {0x99U, 0x88U});

    CCSDS::Packet decoded;
    TEST_VOID(decoded.setMissionProfile(profile));
    std::size_t consumed{};
    TEST_RET(consumed, decoded.deserializeBounded(stream, "PUS:revC:TC"));
    const auto header = std::dynamic_pointer_cast<CCSDS::PusCTcHeader>(
      decoded.getSecondaryHeader());
    return consumed == packetBytes.size() && header && header->getSourceId() == 0x1234U
           && decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x60U, 0x70U});
  });

  tester->unitTest("PUS packet direction, profile, and packet error control mismatches are rejected.", [] {
    const auto tcProfile = CCSDS::makePusProfile(CCSDS::PusRevision::C,
                                                 CCSDS::PacketDirection::Telecommand);
    const auto tmProfile = CCSDS::makePusProfile(CCSDS::PusRevision::C,
                                                 CCSDS::PacketDirection::Telemetry);
    CCSDS::Packet wrongDirection;
    TEST_VOID(wrongDirection.setMissionProfile(tcProfile));
    TEST_VOID(wrongDirection.setSecondaryHeader(std::make_shared<CCSDS::PusCTcHeader>(tcProfile)));
    TEST_VOID(wrongDirection.setApplicationData({1U}));
    if (!wrongDirection.serialize().empty()) return false;

    CCSDS::Packet wrongProfile;
    TEST_VOID(wrongProfile.setMissionProfile(tmProfile));
    if (wrongProfile.setSecondaryHeader(std::make_shared<CCSDS::PusCTcHeader>(tcProfile))) return false;

    CCSDS::Packet wrongPec;
    TEST_VOID(wrongPec.setPrimaryHeader(primary(CCSDS::PacketDirection::Telecommand)));
    TEST_VOID(wrongPec.setMissionProfile(tcProfile));
    TEST_VOID(wrongPec.setSecondaryHeader(std::make_shared<CCSDS::PusCTcHeader>(tcProfile)));
    TEST_VOID(wrongPec.setApplicationData({1U}));
    wrongPec.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    return wrongPec.serialize().empty();
  });

  tester->unitTest("Invalid profiles, versions, reserved bits, timestamp sizes, and spare bytes fail.", [] {
    auto ambiguous = CCSDS::MissionProfile{};
    ambiguous.pusEnabled = true;
    if (CCSDS::validateMissionProfile(ambiguous)) return false;

    auto tcProfile = CCSDS::makePusProfile(CCSDS::PusRevision::A,
                                           CCSDS::PacketDirection::Telecommand);
    tcProfile.sourceIdOctets = 1U;
    CCSDS::PusATcHeader aTc(tcProfile);
    if (aTc.deserialize({0x90U, 0U, 0U, 0U})) return false;

    auto tmProfile = CCSDS::makePusProfile(CCSDS::PusRevision::C,
                                           CCSDS::PacketDirection::Telemetry);
    tmProfile.secondaryHeaderSpareOctets = 1U;
    CCSDS::PusCTmHeader cTm(tmProfile);
    if (cTm.deserialize({0x20U, 1U, 1U, 0U, 0U, 0U, 0U, 1U})) return false;

    tmProfile.telemetryTimestampPresent = true;
    tmProfile.telemetryTimeCode = CCSDS::TimeCodeFormat::Cuc;
    tmProfile.telemetryTimeCodeOctets = 4U;
    CCSDS::PusCTmHeader timed(tmProfile);
    return !timed.setTimestamp({1U, 2U, 3U});
  });
}
