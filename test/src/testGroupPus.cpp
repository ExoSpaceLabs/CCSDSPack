// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "PusSecondaryHeaderFactory.h"
#include "tests.h"
#include <CCSDSPacket.h>
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

namespace {
  class ReservedCustomHeader final : public ccsds::SecondaryHeaderAbstract {
  public:
    [[nodiscard]] ccsds::ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
      m_data = data;
      return true;
    }
    void update(ccsds::DataField *) override {}
    [[nodiscard]] std::uint16_t getSize() const override { return 1U; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }
    [[nodiscard]] std::string getType() const override { return "PUS:mission:custom"; }
#ifndef CCSDS_MCU
    ccsds::ResultBool loadFromConfig(const ccsds::Config &) override { return true; }
#endif
  private:
    std::vector<std::uint8_t> m_data{0U};
  };

  ccsds::PrimaryHeader primary(const ccsds::pus::Direction direction) {
    return {0U,
            static_cast<std::uint8_t>(direction == ccsds::pus::Direction::Telecommand ? 1U : 0U),
            0U, 1U, ccsds::UNSEGMENTED, 0U, 0U};
  }

  bool writeTextFile(const std::string &path, const std::string &contents) {
    std::ofstream file(path, std::ios::trunc);
    file << contents;
    return static_cast<bool>(file);
  }

  std::string pusConfig(const ccsds::pus::Revision revision,
                        const ccsds::pus::Direction direction) {
    const bool tc = direction == ccsds::pus::Direction::Telecommand;
    std::ostringstream config;
    config << "mission_profile:string=pus\n"
           << "ccsds_packet_error_control:string=crc16\n"
           << "data_field_size:int=32\n"
           << "ccsds_version_number:int=0\n"
           << "ccsds_type:bool=" << (tc ? "true" : "false") << '\n'
           << "ccsds_secondary_header_flag:bool=true\n"
           << "ccsds_APID:int=42\n"
           << "ccsds_segmented:bool=false\n"
           << "define_secondary_header:bool=true\n"
           << "secondary_header_type:string="
           << ccsds::pus::selector(revision, direction) << '\n'
           << "pus_revision:string="
           << (revision == ccsds::pus::Revision::A ? "A" : "C") << '\n'
           << "pus_direction:string=" << (tc ? "TC" : "TM") << '\n'
           << "secondary_header_spare_octets:int=0\n"
           << "pus_service_type:uint=17\n"
           << "pus_service_subtype:uint=1\n";
    if (tc) {
      config << "pus_source_id_octets:int="
             << (revision == ccsds::pus::Revision::C ? 2 : 1) << '\n'
             << "pus_acknowledgement_flags:uint=9\n"
             << "pus_source_id:uint=0x42\n";
    } else {
      config << "pus_destination_id_octets:int="
             << (revision == ccsds::pus::Revision::C ? 2 : 1) << '\n'
             << "pus_time_format:string=none\n"
             << "pus_destination_id:uint=0x42\n";
      if (revision == ccsds::pus::Revision::A) {
        config << "pus_a_tm_packet_subcounter_present:bool=true\n"
               << "pus_packet_subcounter:uint=7\n";
      } else {
        config << "pus_message_type_counter:uint=7\n"
               << "pus_time_reference_status:uint=3\n";
      }
    }
    return config.str();
  }
}

void testGroupPus(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupPus: " << description << std::endl;

  tester->unitTest("PUS factory resolves every canonical selector to a fresh matching codec.", [] {
    struct SelectorCase {
      const char *selector;
      ccsds::pus::Revision revision;
      ccsds::pus::Direction direction;
    };

    constexpr std::array<SelectorCase, 4U> cases{{
      {"PUS:revA:TC", ccsds::pus::Revision::A, ccsds::pus::Direction::Telecommand},
      {"PUS:revA:TM", ccsds::pus::Revision::A, ccsds::pus::Direction::Telemetry},
      {"PUS:revC:TC", ccsds::pus::Revision::C, ccsds::pus::Direction::Telecommand},
      {"PUS:revC:TM", ccsds::pus::Revision::C, ccsds::pus::Direction::Telemetry}
    }};

    ccsds::pus::SecondaryHeaderFactory factory;
    for (const auto &entry : cases) {
      const auto profile = ccsds::pus::makeProfile(entry.revision, entry.direction);
      const auto first = factory.create(entry.selector, profile);
      const auto second = factory.create(entry.selector, profile);
      if (!first || !second || first.value() == second.value()) return false;

      const auto header = std::dynamic_pointer_cast<ccsds::pus::SecondaryHeader>(first.value());
      if (!header || header->getType() != entry.selector
          || header->getRevision() != entry.revision
          || header->getDirection() != entry.direction
          || !header->matchesMissionProfile(profile)) return false;
    }

    const auto aTcProfile = ccsds::pus::makeProfile(ccsds::pus::Revision::A,
                                                  ccsds::pus::Direction::Telecommand);
    if (factory.create("PUS:revA:TM", aTcProfile)
        || factory.create("PUS:revB:TC", aTcProfile)
        || factory.typeIsSupported("PUS:reva:TC")) return false;

    return true;
  });

  tester->unitTest("Custom header registration rejects the reserved PUS namespace.", [] {
    ccsds::SecondaryHeaderFactory custom;
    return !custom.registerType<ReservedCustomHeader>()
           && ccsds::pus::SecondaryHeaderFactory::isPusSelector("PUS:mission:custom")
           && !ccsds::pus::SecondaryHeaderFactory::isPusSelector("MissionHeader");
  });

  tester->unitTest("PUS-A TC matches the ECSS-E-70-41A field layout.", [] {
    auto profile = ccsds::pus::makeProfile(ccsds::pus::Revision::A,
                                         ccsds::pus::Direction::Telecommand);
    profile.sourceIdOctets = 1U;
    profile.secondaryHeaderSpareOctets = 1U;
    ccsds::pus::rev_a::TcHeader header(profile, 17U, 2U, 0x55U, 0x0AU);
    const std::vector<std::uint8_t> expected{0x1A, 0x11, 0x02, 0x55, 0x00};
    if (header.serialize() != expected) return false;
    ccsds::pus::rev_a::TcHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getAcknowledgementFlags() == 0x0AU
           && decoded.getServiceType() == 17U && decoded.getServiceSubtype() == 2U
           && decoded.getSourceId() == 0x55U;
  });

  tester->unitTest("PUS-A TM supports its optional subcounter, destination, time, and spare fields.", [] {
    auto profile = ccsds::pus::makeProfile(ccsds::pus::Revision::A,
                                         ccsds::pus::Direction::Telemetry);
    profile.destinationIdOctets = 1U;
    profile.pusATmPacketSubcounterPresent = true;
    profile.telemetryTimestampPresent = true;
    profile.telemetryTimeCode = ccsds::time::Format::Cuc;
    profile.telemetryCuc = {ccsds::time::Epoch::Ccsds1958Tai,
                            ccsds::time::PFieldMode::Implicit, 4U, 0U};
    profile.secondaryHeaderSpareOctets = 1U;
    ccsds::pus::rev_a::TmHeader header(profile, 3U, 25U, 0x44U, 0x7EU,
                                       {0x01020304U, 0U});
    const std::vector<std::uint8_t> expected{
      0x10, 0x03, 0x19, 0x44, 0x7E, 0x01, 0x02, 0x03, 0x04, 0x00};
    if (header.serialize() != expected) return false;
    ccsds::pus::rev_a::TmHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getPacketSubcounter() == 0x44U
           && decoded.getDestinationId() == 0x7EU
           && decoded.getTimestamp() == ccsds::time::CucTime{0x01020304U, 0U};
  });

  tester->unitTest("PUS-C TC matches the ECSS-E-ST-70-41C field layout.", [] {
    const auto profile = ccsds::pus::makeProfile(ccsds::pus::Revision::C,
                                               ccsds::pus::Direction::Telecommand);
    ccsds::pus::rev_c::TcHeader header(profile, 17U, 1U, 0x1234U, 0x0FU);
    const std::vector<std::uint8_t> expected{0x2F, 0x11, 0x01, 0x12, 0x34};
    if (header.serialize() != expected) return false;
    ccsds::pus::rev_c::TcHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getSourceId() == 0x1234U
           && decoded.getAcknowledgementFlags() == 0x0FU;
  });

  tester->unitTest("PUS-C TM without time matches the fixed counter and destination layout.", [] {
    const auto profile = ccsds::pus::makeProfile(ccsds::pus::Revision::C,
                                               ccsds::pus::Direction::Telemetry);
    ccsds::pus::rev_c::TmHeader header(profile, 3U, 25U, 0x1234U, 0xABCDU, 3U);
    const std::vector<std::uint8_t> expected{0x23, 0x03, 0x19, 0x12, 0x34, 0xAB, 0xCD};
    if (header.serialize() != expected) return false;
    ccsds::pus::rev_c::TmHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getMessageTypeCounter() == 0x1234U
           && decoded.getDestinationId() == 0xABCDU
           && decoded.getTimeReferenceStatus() == 3U;
  });

  tester->unitTest("PUS-C TM timestamp and spare sizes are profile-derived.", [] {
    auto profile = ccsds::pus::makeProfile(ccsds::pus::Revision::C,
                                         ccsds::pus::Direction::Telemetry);
    profile.telemetryTimestampPresent = true;
    profile.telemetryTimeCode = ccsds::time::Format::Cuc;
    profile.telemetryCuc = {ccsds::time::Epoch::Ccsds1958Tai,
                            ccsds::time::PFieldMode::Implicit, 4U, 0U};
    profile.secondaryHeaderSpareOctets = 1U;
    ccsds::pus::rev_c::TmHeader header(profile, 5U, 1U, 7U, 0x0102U, 5U,
                              {0x11223344U, 0U});
    const std::vector<std::uint8_t> expected{
      0x25, 0x05, 0x01, 0x00, 0x07, 0x01, 0x02, 0x11, 0x22, 0x33, 0x44, 0x00};
    if (header.getSize() != expected.size() || header.serialize() != expected) return false;
    ccsds::pus::rev_c::TmHeader decoded(profile);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getTimestamp() == ccsds::time::CucTime{0x11223344U, 0U};
  });

  tester->unitTest("Canonical PUS selector round-trips a packet and respects its boundary.", [] {
    const auto profile = ccsds::pus::makeProfile(ccsds::pus::Revision::C,
                                               ccsds::pus::Direction::Telecommand);
    ccsds::Packet source;
    TEST_VOID(source.setPrimaryHeader(primary(ccsds::pus::Direction::Telecommand)));
    TEST_VOID(source.setMissionProfile(profile));
    TEST_VOID(source.setSecondaryHeader(
      std::make_shared<ccsds::pus::rev_c::TcHeader>(profile, 17U, 1U, 0x1234U, 0x09U)));
    TEST_VOID(source.setApplicationData({0x60U, 0x70U}));
    const auto packetBytes = serializedPacket(source);
    auto stream = packetBytes;
    stream.insert(stream.end(), {0x99U, 0x88U});

    ccsds::Packet decoded;
    TEST_VOID(decoded.setMissionProfile(profile));
    std::size_t consumed{};
    TEST_RET(consumed, decoded.deserializeBounded(stream, "PUS:revC:TC"));
    const auto header = std::dynamic_pointer_cast<ccsds::pus::rev_c::TcHeader>(
      decoded.getSecondaryHeader());
    return consumed == packetBytes.size() && header && header->getSourceId() == 0x1234U
           && decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x60U, 0x70U});
  });

  tester->unitTest("PUS packet direction, profile, and packet error control mismatches are rejected.", [] {
    const auto tcProfile = ccsds::pus::makeProfile(ccsds::pus::Revision::C,
                                                 ccsds::pus::Direction::Telecommand);
    const auto tmProfile = ccsds::pus::makeProfile(ccsds::pus::Revision::C,
                                                 ccsds::pus::Direction::Telemetry);
    ccsds::Packet wrongDirection;
    TEST_VOID(wrongDirection.setMissionProfile(tcProfile));
    TEST_VOID(wrongDirection.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TcHeader>(tcProfile)));
    TEST_VOID(wrongDirection.setApplicationData({1U}));
    const auto directionResult = wrongDirection.serialize();
    if (directionResult
        || directionResult.error().code() != ccsds::INVALID_HEADER_DATA) return false;

    ccsds::Packet wrongProfile;
    TEST_VOID(wrongProfile.setMissionProfile(tmProfile));
    if (wrongProfile.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TcHeader>(tcProfile))) return false;

    ccsds::Packet wrongPec;
    TEST_VOID(wrongPec.setPrimaryHeader(primary(ccsds::pus::Direction::Telecommand)));
    TEST_VOID(wrongPec.setMissionProfile(tcProfile));
    TEST_VOID(wrongPec.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TcHeader>(tcProfile)));
    TEST_VOID(wrongPec.setApplicationData({1U}));
    wrongPec.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    const auto pecResult = wrongPec.serialize();
    return !pecResult && pecResult.error().code() == ccsds::INVALID_DATA;
  });

  tester->unitTest("Invalid profiles, versions, reserved bits, timestamp sizes, and spare bytes fail.", [] {
    auto ambiguous = ccsds::MissionProfile{};
    ambiguous.pusEnabled = true;
    if (ccsds::validateMissionProfile(ambiguous)) return false;

    auto tcProfile = ccsds::pus::makeProfile(ccsds::pus::Revision::A,
                                           ccsds::pus::Direction::Telecommand);
    tcProfile.sourceIdOctets = 1U;
    ccsds::pus::rev_a::TcHeader aTc(tcProfile);
    if (aTc.deserialize({0x90U, 0U, 0U, 0U})) return false;

    auto tmProfile = ccsds::pus::makeProfile(ccsds::pus::Revision::C,
                                           ccsds::pus::Direction::Telemetry);
    tmProfile.secondaryHeaderSpareOctets = 1U;
    ccsds::pus::rev_c::TmHeader cTm(tmProfile);
    if (cTm.deserialize({0x20U, 1U, 1U, 0U, 0U, 0U, 0U, 1U})) return false;

    tmProfile.telemetryTimestampPresent = true;
    tmProfile.telemetryTimeCode = ccsds::time::Format::Cuc;
    tmProfile.telemetryCuc = {ccsds::time::Epoch::Ccsds1958Tai,
                              ccsds::time::PFieldMode::Implicit, 4U, 0U};
    ccsds::pus::rev_c::TmHeader timed(tmProfile);
    return !timed.setTimestamp({0x100000000ULL, 0U});
  });

  tester->unitTest("Basic CUC encodes numeric counters with implicit and explicit P-fields.", [] {
    const ccsds::time::CucConfiguration explicitCuc{
      ccsds::time::Epoch::Ccsds1958Tai,
      ccsds::time::PFieldMode::Explicit, 4U, 3U};
    const std::vector<std::uint8_t> expected{
      0x1FU, 0x01U, 0x02U, 0x03U, 0x04U, 0xA0U, 0xB0U, 0xC0U};
    const auto encoded = ccsds::time::serialize(
      {0x01020304U, 0xA0B0C0U}, explicitCuc);
    if (!encoded || encoded.value() != expected) return false;
    const auto decoded = ccsds::time::deserialize(expected, explicitCuc);
    if (!decoded
        || decoded.value() != ccsds::time::CucTime{0x01020304U, 0xA0B0C0U}) return false;

    const ccsds::time::CucConfiguration implicitCuc{
      ccsds::time::Epoch::AgencyDefined,
      ccsds::time::PFieldMode::Implicit, 2U, 1U};
    const auto implicit = ccsds::time::serialize({0x1234U, 0x80U}, implicitCuc);
    return implicit
           && implicit.value() == std::vector<std::uint8_t>({0x12U, 0x34U, 0x80U});
  });

  tester->unitTest("CUC rejects mismatched P-fields, invalid widths, and counter overflow.", [] {
    const ccsds::time::CucConfiguration configuration{
      ccsds::time::Epoch::Ccsds1958Tai,
      ccsds::time::PFieldMode::Explicit, 1U, 1U};
    if (ccsds::time::deserialize({0x20U, 0x01U, 0x02U}, configuration)) return false;
    if (ccsds::time::serialize({0x100U, 0U}, configuration)) return false;
    auto invalid = configuration;
    invalid.coarseOctets = 0U;
    return !ccsds::time::validate(invalid);
  });

  tester->unitTest("Configuration creates every PUS revision and direction explicitly.", [] {
    constexpr std::array<std::pair<ccsds::pus::Revision, ccsds::pus::Direction>, 4U> cases{{
      {ccsds::pus::Revision::A, ccsds::pus::Direction::Telecommand},
      {ccsds::pus::Revision::A, ccsds::pus::Direction::Telemetry},
      {ccsds::pus::Revision::C, ccsds::pus::Direction::Telecommand},
      {ccsds::pus::Revision::C, ccsds::pus::Direction::Telemetry}
    }};
    for (std::size_t index = 0U; index < cases.size(); ++index) {
      const auto [revision, direction] = cases[index];
      const auto path = "ccsdspack_pus_profile_" + std::to_string(index) + ".cfg";
      if (!writeTextFile(path, pusConfig(revision, direction))) return false;
      ccsds::Packet packet;
      const auto loaded = packet.loadFromConfigFile(path);
      std::remove(path.c_str());
      if (!loaded || !packet.getSecondaryHeader()
          || packet.getSecondaryHeader()->getType() != ccsds::pus::selector(revision, direction)) {
        return false;
      }
    }

    const std::string legacyPath{"ccsdspack_legacy_pus.cfg"};
    if (!writeTextFile(legacyPath,
                       "mission_profile:string=pus\n"
                       "ccsds_packet_error_control:string=crc16\n"
                       "pus_version:int=2\n")) return false;
    ccsds::Packet legacy;
    const auto result = legacy.loadFromConfigFile(legacyPath);
    std::remove(legacyPath.c_str());
    return !result && result.error().code() == ccsds::CONFIG_FILE_ERROR;
  });

  tester->unitTest("Manager decodes PUS packets with the template mission profile.", [] {
    auto profile = ccsds::pus::makeProfile(ccsds::pus::Revision::C,
                                           ccsds::pus::Direction::Telemetry);
    profile.telemetryTimestampPresent = true;
    profile.telemetryTimeCode = ccsds::time::Format::Cuc;
    profile.telemetryCuc = {ccsds::time::Epoch::Ccsds1958Tai,
                            ccsds::time::PFieldMode::Explicit, 4U, 2U};
    ccsds::Packet source;
    TEST_VOID(source.setPrimaryHeader(primary(ccsds::pus::Direction::Telemetry)));
    TEST_VOID(source.setMissionProfile(profile));
    TEST_VOID(source.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TmHeader>(
      profile, 3U, 25U, 4U, 0x1234U, 2U, ccsds::time::CucTime{0x01020304U, 0xA0B0U})));
    TEST_VOID(source.setApplicationData({0xAAU, 0x55U}));
    const auto bytes = source.serialize();
    if (!bytes) return false;

    ccsds::Packet packetTemplate = source;
    ccsds::Manager manager;
    TEST_VOID(manager.setPacketTemplate(std::move(packetTemplate)));
    TEST_VOID(manager.load(bytes.value()));
    const auto packets = manager.getPackets();
    if (packets.size() != 1U) return false;
    const auto header = std::dynamic_pointer_cast<const ccsds::pus::rev_c::TmHeader>(
      packets.front().getSecondaryHeader());
    return header && header->getTimestamp()
           == ccsds::time::CucTime{0x01020304U, 0xA0B0U};
  });
}
