// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "PusSecondaryHeaderFactory.h"
#include "tests.h"
#include <CCSDSPack.h>
#include <ccsdspack/c/cuc.h>
#include <algorithm>
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

  ccsds::PrimaryHeader primary(const ccsds::PacketDirection direction) {
    return {0U, ccsds::packetTypeForDirection(direction), 0U, 1U,
            ccsds::UNSEGMENTED, 0U, 0U};
  }

  bool writeTextFile(const std::string &path, const std::string &contents) {
    std::ofstream file(path, std::ios::trunc);
    file << contents;
    return static_cast<bool>(file);
  }

  std::string pusConfig(const ccsds::pus::Revision revision,
                        const ccsds::PacketDirection direction) {
    const bool tc = direction == ccsds::PacketDirection::Telecommand;
    std::ostringstream config;
    config << "ccsds_packet_error_control:string=crc16\n"
           << "data_field_size:int=32\n"
           << "ccsds_version_number:int=0\n"
           << "ccsds_APID:int=42\n"
           << "ccsds_segmented:bool=false\n"
           << "define_secondary_header:bool=true\n"
           << "secondary_header_type:string="
           << ccsds::pus::selector(revision, direction) << '\n'
           << "secondary_header_spare_octets:int=0\n"
           << "pus_service_type:uint=17\n"
           << "pus_service_subtype:uint=1\n";
    if (tc) {
      if (revision == ccsds::pus::Revision::A)
        config << "pus_source_id_octets:int=1\n";
      config << "pus_acknowledgement_flags:uint=9\n"
             << "pus_source_id:uint=0x42\n";
    } else {
      if (revision == ccsds::pus::Revision::A)
        config << "pus_destination_id_octets:int=1\n";
      config << "pus_time_format:string=none\n"
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

  tester->unitTest("PUS factory resolves canonical selectors to fresh intrinsic identities.", [] {
    struct SelectorCase {
      const char *selector;
      ccsds::pus::Revision revision;
      ccsds::PacketDirection direction;
    };
    constexpr std::array<SelectorCase, 4U> cases{{
      {"PUS:revA:TC", ccsds::pus::Revision::A, ccsds::PacketDirection::Telecommand},
      {"PUS:revA:TM", ccsds::pus::Revision::A, ccsds::PacketDirection::Telemetry},
      {"PUS:revC:TC", ccsds::pus::Revision::C, ccsds::PacketDirection::Telecommand},
      {"PUS:revC:TM", ccsds::pus::Revision::C, ccsds::PacketDirection::Telemetry}
    }};

    ccsds::pus::SecondaryHeaderFactory factory;
    for (const auto &entry : cases) {
      const auto first = factory.create(entry.selector);
      const auto second = factory.create(entry.revision, entry.direction);
      if (!first || !second || first.value() == second.value()) return false;
      const auto *header = static_cast<const ccsds::pus::SecondaryHeader *>(first.value().get());
      if (!header || header->getType() != entry.selector
          || header->getRevision() != entry.revision
          || header->getDirection() != entry.direction) return false;
    }
    return !factory.create("PUS:revB:TC") && !factory.typeIsSupported("PUS:reva:TC");
  });

  tester->unitTest("Concrete PUS classes own revision and direction independently of tailoring.", [] {
    const ccsds::pus::rev_a::TcHeader aTc;
    const ccsds::pus::rev_a::TmHeader aTm;
    const ccsds::pus::rev_c::TcHeader cTc;
    const ccsds::pus::rev_c::TmHeader cTm;
    return aTc.getRevision() == ccsds::pus::Revision::A
           && aTc.getDirection() == ccsds::PacketDirection::Telecommand
           && aTm.getRevision() == ccsds::pus::Revision::A
           && aTm.getDirection() == ccsds::PacketDirection::Telemetry
           && cTc.getRevision() == ccsds::pus::Revision::C
           && cTc.getDirection() == ccsds::PacketDirection::Telecommand
           && cTm.getRevision() == ccsds::pus::Revision::C
           && cTm.getDirection() == ccsds::PacketDirection::Telemetry;
  });

  tester->unitTest("Custom header registration rejects the reserved PUS namespace.", [] {
    ccsds::SecondaryHeaderFactory custom;
    return !custom.registerType<ReservedCustomHeader>()
           && ccsds::pus::SecondaryHeaderFactory::isPusSelector("PUS:mission:custom")
           && !ccsds::pus::SecondaryHeaderFactory::isPusSelector("MissionHeader");
  });

  tester->unitTest("PUS-A TC matches the ECSS-E-70-41A field layout with optional tailoring.", [] {
    ccsds::pus::rev_a::TcTailoring tailoring;
    tailoring.sourceIdOctets = 1U;
    tailoring.secondaryHeaderSpareOctets = 1U;
    ccsds::pus::rev_a::TcHeader header(tailoring, 17U, 2U, 0x55U, 0x0AU);
    const std::vector<std::uint8_t> expected{0x1A, 0x11, 0x02, 0x55, 0x00};
    if (header.serialize() != expected) return false;
    ccsds::pus::rev_a::TcHeader decoded(tailoring);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getAcknowledgementFlags() == 0x0AU
           && decoded.getServiceType() == 17U && decoded.getServiceSubtype() == 2U
           && decoded.getSourceId() == 0x55U;
  });

  tester->unitTest("PUS-A TM tailoring controls subcounter, destination, time, and spare fields.", [] {
    ccsds::pus::rev_a::TmTailoring tailoring;
    tailoring.destinationIdOctets = 1U;
    tailoring.packetSubcounterPresent = true;
    tailoring.timestampPresent = true;
    tailoring.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                     ccsds::time::PFieldMode::Implicit, 4U, 0U};
    tailoring.secondaryHeaderSpareOctets = 1U;
    ccsds::pus::rev_a::TmHeader header(tailoring, 3U, 25U, 0x44U, 0x7EU,
                                      {0x01020304U, 0U});
    const std::vector<std::uint8_t> expected{
      0x10, 0x03, 0x19, 0x44, 0x7E, 0x01, 0x02, 0x03, 0x04, 0x00};
    if (header.serialize() != expected) return false;
    ccsds::pus::rev_a::TmHeader decoded(tailoring);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getPacketSubcounter() == 0x44U
           && decoded.getDestinationId() == 0x7EU
           && decoded.getTimestamp() == ccsds::time::CucTime{0x01020304U, 0U};
  });

  tester->unitTest("PUS-C TC default tailoring matches the ECSS-E-ST-70-41C field layout.", [] {
    ccsds::pus::rev_c::TcHeader header(17U, 1U, 0x1234U, 0x0FU);
    const std::vector<std::uint8_t> expected{0x2F, 0x11, 0x01, 0x12, 0x34};
    if (header.serialize() != expected || header.getSourceIdOctets() != 2U) return false;
    ccsds::pus::rev_c::TcHeader decoded;
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getSourceId() == 0x1234U
           && decoded.getAcknowledgementFlags() == 0x0FU;
  });

  tester->unitTest("PUS-C TM default tailoring matches the fixed counter and destination layout.", [] {
    ccsds::pus::rev_c::TmHeader header(3U, 25U, 0x1234U, 0xABCDU, 3U);
    const std::vector<std::uint8_t> expected{0x23, 0x03, 0x19, 0x12, 0x34, 0xAB, 0xCD};
    if (header.serialize() != expected || header.getDestinationIdOctets() != 2U) return false;
    ccsds::pus::rev_c::TmHeader decoded;
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getMessageTypeCounter() == 0x1234U
           && decoded.getDestinationId() == 0xABCDU
           && decoded.getTimeReferenceStatus() == 3U;
  });

  tester->unitTest("PUS-C TM timestamp and spare sizes are optional tailoring only.", [] {
    ccsds::pus::rev_c::TmTailoring tailoring;
    tailoring.timestampPresent = true;
    tailoring.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                     ccsds::time::PFieldMode::Implicit, 4U, 0U};
    tailoring.secondaryHeaderSpareOctets = 1U;
    ccsds::pus::rev_c::TmHeader header(tailoring, 5U, 1U, 7U, 0x0102U, 5U,
                                      {0x11223344U, 0U});
    const std::vector<std::uint8_t> expected{
      0x25, 0x05, 0x01, 0x00, 0x07, 0x01, 0x02, 0x11, 0x22, 0x33, 0x44, 0x00};
    if (header.getSize() != expected.size() || header.serialize() != expected) return false;
    ccsds::pus::rev_c::TmHeader decoded(tailoring);
    TEST_VOID(decoded.deserialize(expected));
    return decoded.getTimestamp() == ccsds::time::CucTime{0x11223344U, 0U};
  });

  tester->unitTest("Installing a directional PUS header synchronizes CCSDS Packet Type.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(primary(ccsds::PacketDirection::Telemetry)));
    TEST_VOID(packet.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TcHeader>()));
    return packet.getDirection() == ccsds::PacketDirection::Telecommand
           && packet.getPrimaryHeader().getType() == 1U
           && packet.getSecondaryHeader()->getDirection() == ccsds::PacketDirection::Telecommand;
  });

  tester->unitTest("Packet serialization rejects later primary-header direction corruption.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(primary(ccsds::PacketDirection::Telemetry)));
    TEST_VOID(packet.setSecondaryHeader(
      std::make_shared<ccsds::pus::rev_c::TcHeader>(17U, 1U, 0x1234U, 0x09U)));
    TEST_VOID(packet.setApplicationData({1U}));
    TEST_VOID(packet.getPrimaryHeader().setType(0U));
    const auto result = packet.serialize();
    return !result && result.error().code() == ccsds::INVALID_HEADER_DATA;
  });

  tester->unitTest("Packet error control remains independent of PUS tailoring.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(primary(ccsds::PacketDirection::Telecommand)));
    TEST_VOID(packet.setSecondaryHeader(
      std::make_shared<ccsds::pus::rev_c::TcHeader>(17U, 1U, 0x1234U, 0x09U)));
    TEST_VOID(packet.setApplicationData({1U}));
    packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    const auto result = packet.serialize();
    return result && packet.getPacketErrorControlMode() == ccsds::PacketErrorControlMode::None
           && packet.getCRCVectorBytes().empty();
  });

  tester->unitTest("Preinstalled PUS header acts as deserialize schema with default tailoring.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setPrimaryHeader(primary(ccsds::PacketDirection::Telecommand)));
    TEST_VOID(source.setSecondaryHeader(
      std::make_shared<ccsds::pus::rev_c::TcHeader>(17U, 1U, 0x1234U, 0x09U)));
    TEST_VOID(source.setApplicationData({0x60U, 0x70U}));
    const auto wire = source.serialize();
    if (!wire) return false;

    ccsds::Packet decoded;
    TEST_VOID(decoded.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TcHeader>()));
    TEST_VOID(decoded.deserialize(wire.value()));
    const auto header = std::static_pointer_cast<ccsds::pus::rev_c::TcHeader>(
      decoded.getSecondaryHeader());
    return header->getSourceId() == 0x1234U
           && decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x60U, 0x70U});
  });

  tester->unitTest("Typed PUS deserialize constructs the header schema automatically.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setPrimaryHeader(primary(ccsds::PacketDirection::Telemetry)));
    TEST_VOID(source.setSecondaryHeader(
      std::make_shared<ccsds::pus::rev_c::TmHeader>(3U, 25U, 7U, 0x1234U, 2U)));
    TEST_VOID(source.setApplicationData({0xAAU}));
    const auto wire = source.serialize();
    if (!wire) return false;

    ccsds::Packet decoded;
    TEST_VOID(decoded.deserialize<ccsds::pus::rev_c::TmHeader>(wire.value()));
    const auto header = std::static_pointer_cast<ccsds::pus::rev_c::TmHeader>(
      decoded.getSecondaryHeader());
    return header->getMessageTypeCounter() == 7U && header->getDestinationId() == 0x1234U
           && decoded.getDirection() == ccsds::PacketDirection::Telemetry;
  });

  tester->unitTest("Typed PUS deserialize accepts explicit tailoring when needed.", [] {
    ccsds::pus::rev_c::TmTailoring tailoring;
    tailoring.timestampPresent = true;
    tailoring.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                     ccsds::time::PFieldMode::Explicit, 4U, 2U};
    ccsds::Packet source;
    TEST_VOID(source.setPrimaryHeader(primary(ccsds::PacketDirection::Telemetry)));
    TEST_VOID(source.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TmHeader>(
      tailoring, 3U, 25U, 4U, 0x1234U, 2U,
      ccsds::time::CucTime{0x01020304U, 0xA0B0U})));
    TEST_VOID(source.setApplicationData({0xAAU, 0x55U}));
    const auto wire = source.serialize();
    if (!wire) return false;

    ccsds::Packet decoded;
    TEST_VOID(decoded.deserialize<ccsds::pus::rev_c::TmHeader>(wire.value(), tailoring));
    const auto header = std::static_pointer_cast<ccsds::pus::rev_c::TmHeader>(
      decoded.getSecondaryHeader());
    return header->getTimestamp() == ccsds::time::CucTime{0x01020304U, 0xA0B0U};
  });

  tester->unitTest("Canonical selector parsing remains available and bounded.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setPrimaryHeader(primary(ccsds::PacketDirection::Telecommand)));
    TEST_VOID(source.setSecondaryHeader(
      std::make_shared<ccsds::pus::rev_c::TcHeader>(17U, 1U, 0x1234U, 0x09U)));
    TEST_VOID(source.setApplicationData({0x60U, 0x70U}));
    const auto packetBytes = serializedPacket(source);
    auto stream = packetBytes;
    stream.insert(stream.end(), {0x99U, 0x88U});

    ccsds::Packet decoded;
    std::size_t consumed{};
    TEST_RET(consumed, decoded.deserializeBounded(stream, "PUS:revC:TC"));
    const auto header = std::static_pointer_cast<ccsds::pus::rev_c::TcHeader>(
      decoded.getSecondaryHeader());
    return consumed == packetBytes.size() && header->getSourceId() == 0x1234U;
  });

  tester->unitTest("Invalid PUS tailoring, reserved bits, time overflow, and spare bytes fail.", [] {
    ccsds::pus::rev_a::TcTailoring invalidA;
    invalidA.sourceIdOctets = 3U;
    ccsds::pus::rev_a::TcHeader invalidHeader(invalidA, 1U, 1U);
    if (!invalidHeader.serialize().empty()) return false;

    ccsds::pus::rev_a::TcTailoring aTailoring;
    aTailoring.sourceIdOctets = 1U;
    ccsds::pus::rev_a::TcHeader aTc(aTailoring);
    if (aTc.deserialize({0x90U, 0U, 0U, 0U})) return false;

    ccsds::pus::rev_c::TmTailoring spareTailoring;
    spareTailoring.secondaryHeaderSpareOctets = 1U;
    ccsds::pus::rev_c::TmHeader cTm(spareTailoring);
    if (cTm.deserialize({0x20U, 1U, 1U, 0U, 0U, 0U, 0U, 1U})) return false;

    ccsds::pus::rev_c::TmTailoring timedTailoring;
    timedTailoring.timestampPresent = true;
    timedTailoring.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                          ccsds::time::PFieldMode::Implicit, 4U, 0U};
    ccsds::pus::rev_c::TmHeader timed(timedTailoring);
    return !timed.setTimestamp({0x100000000ULL, 0U});
  });

  tester->unitTest("Basic CUC encodes numeric counters with implicit and explicit P-fields.", [] {
    const ccsds::time::CucConfiguration explicitCuc{
      ccsds::time::Epoch::Ccsds1958Tai,
      ccsds::time::PFieldMode::Explicit, 4U, 3U};
    const std::vector<std::uint8_t> expected{
      0x1FU, 0x01U, 0x02U, 0x03U, 0x04U, 0xA0U, 0xB0U, 0xC0U};
    const auto encoded = ccsds::time::serialize({0x01020304U, 0xA0B0C0U}, explicitCuc);
    if (!encoded || encoded.value() != expected) return false;
    const auto decoded = ccsds::time::deserialize(expected, explicitCuc);
    if (!decoded || decoded.value() != ccsds::time::CucTime{0x01020304U, 0xA0B0C0U}) return false;

    const ccsds::time::CucConfiguration implicitCuc{
      ccsds::time::Epoch::AgencyDefined,
      ccsds::time::PFieldMode::Implicit, 2U, 1U};
    const auto implicit = ccsds::time::serialize({0x1234U, 0x80U}, implicitCuc);
    return implicit && implicit.value() == std::vector<std::uint8_t>({0x12U, 0x34U, 0x80U});
  });

  tester->unitTest("Bounded C++ CUC produces fixed CCSDS 301.0 basic vectors.", [] {
    using namespace ccsds::time;
    const CucConfiguration standard{
      Epoch::Ccsds1958Tai, PFieldMode::Explicit, 4U, 3U};
    const CucConfiguration agency{
      Epoch::AgencyDefined, PFieldMode::Explicit, 2U, 1U};
    const CucConfiguration minimum{
      Epoch::Ccsds1958Tai, PFieldMode::Explicit, 1U, 0U};
    struct Case {
      CucConfiguration cfg;
      CucTime value;
      std::vector<std::uint8_t> expected;
    };
    const std::vector<Case> vectors{
      {standard, {0x01020304U, 0xA0B0C0U},
        {0x1FU, 1U, 2U, 3U, 4U, 0xA0U, 0xB0U, 0xC0U}},
      {agency, {0x1234U, 0x80U}, {0x25U, 0x12U, 0x34U, 0x80U}},
      {minimum, {0x7FU, 0U}, {0x10U, 0x7FU}}
    };
    for (const auto &entry : vectors) {
      std::array<std::uint8_t, 8U> bytes{};
      const auto written = serialize(
        entry.value, entry.cfg, bytes.data(), bytes.size());
      if (!written || written.value() != entry.expected.size()
          || !std::equal(entry.expected.begin(), entry.expected.end(),
                         bytes.begin())) return false;
      const auto decoded = deserialize(
        bytes.data(), written.value(), entry.cfg);
      if (!decoded || decoded.value() != entry.value) return false;
    }
    return true;
  });

  tester->unitTest("Bounded C++ CUC matches C11 on all 64 basic layouts.", [] {
    using namespace ccsds::time;
    for (const auto epoch : {Epoch::Ccsds1958Tai, Epoch::AgencyDefined}) {
      for (const auto mode : {PFieldMode::Implicit, PFieldMode::Explicit}) {
        for (std::uint8_t coarse = 1U; coarse <= 4U; ++coarse) {
          for (std::uint8_t fine = 0U; fine <= 3U; ++fine) {
            const CucConfiguration cfg{epoch, mode, coarse, fine};
            const ccsds_cuc_config_t cCfg{
              static_cast<ccsds_cuc_epoch_t>(epoch),
              static_cast<ccsds_cuc_pfield_mode_t>(mode), coarse, fine};
            const auto coarseMax = (UINT64_C(1) << (coarse * 8U)) - 1U;
            const auto fineMax = fine == 0U ? 0U
              : (UINT64_C(1) << (fine * 8U)) - 1U;
            const CucTime val{UINT64_C(0xDEADBEEF) & coarseMax,
                              UINT64_C(0x00AABBCC) & fineMax};
            const ccsds_cuc_time_t cVal{val.coarse, val.fine};
            std::array<std::uint8_t, 8U> rawC{};
            std::array<std::uint8_t, 8U> rawCpp{};
            std::size_t cWritten = 0U;
            if (ccsds_cuc_encode(&cVal, &cCfg, rawC.data(), rawC.size(),
                                 &cWritten) != CCSDS_STATUS_OK) return false;
            const auto cppWritten = serialize(
              val, cfg, rawCpp.data(), rawCpp.size());
            const auto legacy = serialize(val, cfg);
            if (!cppWritten || cppWritten.value() != cWritten
                || !legacy || legacy.value().size() != cWritten
                || !std::equal(rawC.begin(), rawC.begin() + cWritten,
                               rawCpp.begin())
                || !std::equal(rawC.begin(), rawC.begin() + cWritten,
                               legacy.value().begin())) return false;
            const auto decoded = deserialize(rawCpp.data(), cWritten, cfg);
            const auto legacyDecoded = deserialize(legacy.value(), cfg);
            if (!decoded || decoded.value() != val
                || !legacyDecoded || legacyDecoded.value() != val) return false;
          }
        }
      }
    }
    return true;
  });

  tester->unitTest("Bounded CUC rejects invalid output without partial writes.", [] {
    using namespace ccsds::time;
    const CucConfiguration cfg{
      Epoch::Ccsds1958Tai, PFieldMode::Explicit, 1U, 1U};
    std::array<std::uint8_t, 8U> output{};
    output.fill(0xA5U);
    const auto saved = output;
    const auto tooSmall = serialize(
      {0x12U, 0x34U}, cfg, output.data(), 2U);
    const auto nullOut = serialize({0x12U, 0x34U}, cfg, nullptr, 3U);
    const auto overflow = serialize({0x100U, 0U}, cfg, output.data(), output.size());
    const auto fineOverflow = serialize({0U, 0x100U}, cfg, output.data(), output.size());
    auto invalid = cfg;
    invalid.coarseOctets = 0U;
    const auto badConfig = serialize({0U, 0U}, invalid, output.data(), output.size());
    if (tooSmall || nullOut || overflow || fineOverflow || badConfig
        || output != saved
        || tooSmall.error().code() != ccsds::INVALID_DATA
        || nullOut.error().code() != ccsds::NULL_POINTER
        || overflow.error().message() != "CUC coarse time does not fit the configured width."
        || fineOverflow.error().message() != "CUC fine time does not fit the configured width."
        || badConfig.error().message() != "Basic CUC coarse time requires 1 to 4 octets.") {
      return false;
    }
    const std::array<std::uint8_t, 3U> incorrectPfield{0x20U, 0x12U, 0x34U};
    const auto mismatch = deserialize(incorrectPfield.data(), incorrectPfield.size(), cfg);
    const auto badSize = deserialize(incorrectPfield.data(), 2U, cfg);
    const auto nullIn = deserialize(nullptr, 3U, cfg);
    return !mismatch && !badSize && !nullIn
        && mismatch.error().message() == "CUC P-field does not match the configured epoch or widths."
        && badSize.error().message() == "CUC encoded size does not match the configured layout."
        && nullIn.error().code() == ccsds::NULL_POINTER;
  });

  tester->unitTest("C++ CUC encoding delegates all 64 supported layouts to the C11 codec.", [] {
    for (const auto epoch : {ccsds::time::Epoch::Ccsds1958Tai,
                             ccsds::time::Epoch::AgencyDefined}) {
      for (const auto pfield : {ccsds::time::PFieldMode::Implicit,
                                ccsds::time::PFieldMode::Explicit}) {
        for (std::uint8_t coarse = 1U; coarse <= 4U; ++coarse) {
          for (std::uint8_t fine = 0U; fine <= 3U; ++fine) {
            const ccsds::time::CucConfiguration cfg{epoch, pfield, coarse, fine};
            const ccsds_cuc_config_t core{
              static_cast<ccsds_cuc_epoch_t>(epoch),
              static_cast<ccsds_cuc_pfield_mode_t>(pfield), coarse, fine
            };
            const std::uint64_t coarseMax =
              (UINT64_C(1) << (8U * coarse)) - UINT64_C(1);
            const std::uint64_t fineMax =
              fine == 0U ? 0U : ((UINT64_C(1) << (8U * fine)) - UINT64_C(1));
            const ccsds::time::CucTime input{
              UINT64_C(0x01020304) & coarseMax,
              UINT64_C(0x00A0B0C0) & fineMax
            };
            const ccsds_cuc_time_t coreValue{input.coarse, input.fine};
            std::array<std::uint8_t, 8U> coreBytes{};
            std::size_t written = 0U;
            if (ccsds_cuc_encode(&coreValue, &core,
                                 coreBytes.data(), coreBytes.size(),
                                 &written) != CCSDS_STATUS_OK) return false;
            const auto wrapped = ccsds::time::serialize(input, cfg);
            if (!wrapped || wrapped.value().size() != written
                || !std::equal(wrapped.value().begin(), wrapped.value().end(),
                               coreBytes.begin())) return false;
          }
        }
      }
    }
    return true;
  });

  tester->unitTest("C++ CUC encoding preserves the C11 validation error categories.", [] {
    using ccsds::time::CucConfiguration;
    using ccsds::time::CucTime;
    const CucConfiguration valid{
      ccsds::time::Epoch::Ccsds1958Tai,
      ccsds::time::PFieldMode::Explicit, 1U, 1U};
    const auto overflow = ccsds::time::serialize(CucTime{256U, 0U}, valid);
    if (overflow || overflow.error().code() != ccsds::INVALID_DATA
        || overflow.error().message() !=
           "CUC coarse time does not fit the configured width.") return false;
    auto invalid = valid;
    invalid.coarseOctets = 0U;
    const auto rejected = ccsds::time::serialize(CucTime{1U, 0U}, invalid);
    return !rejected && rejected.error().code() == ccsds::INVALID_DATA
      && rejected.error().message() ==
           "Basic CUC coarse time requires 1 to 4 octets.";
  });

  tester->unitTest("PUS TM encoding delegates invalid tailoring to the C11 codecs.", [] {
    ccsds::pus::rev_a::TmTailoring badA{};
    badA.destinationIdOctets = 3U; // C11 PUS-A accepts 0, 1, 2 or 4.
    ccsds::pus::rev_a::TmHeader a(badA);
    if (!a.serialize().empty()) return false;

    ccsds::pus::rev_c::TmTailoring badC{};
    badC.timestampPresent = false;
    badC.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                ccsds::time::PFieldMode::Implicit, 4U, 0U};
    ccsds::pus::rev_c::TmHeader c(badC);
    if (!c.serialize().empty()) return false;

    const ccsds_pus_a_tm_tailoring_t cA{
      3U, 0U, 0U, {}, 0U
    };
    const ccsds_pus_c_tm_tailoring_t cC{
      0U, {CCSDS_CUC_EPOCH_CCSDS_1958_TAI,
           CCSDS_CUC_PFIELD_IMPLICIT, 4U, 0U}, 0U
    };
    return ccsds_pus_a_tm_validate_tailoring(&cA)
             == CCSDS_STATUS_PUS_INVALID_IDENTIFIER_WIDTH
        && ccsds_pus_c_tm_validate_tailoring(&cC)
             == CCSDS_STATUS_PUS_DISABLED_TIME_CONFIG;
  });

  tester->unitTest("PUS C++ identifier setters use C11 field-width rules.", [] {
    ccsds::pus::rev_a::TcTailoring invalidTc{};
    invalidTc.sourceIdOctets = 3U;
    ccsds::pus::rev_a::TcHeader tc(invalidTc);
    if (tc.setSourceId(1U)) return false;

    ccsds::pus::rev_a::TmTailoring invalidTm{};
    invalidTm.destinationIdOctets = 3U;
    ccsds::pus::rev_a::TmHeader tm(invalidTm);
    if (tm.setDestinationId(1U)) return false;

    ccsds::pus::rev_a::TcTailoring valid{};
    valid.sourceIdOctets = 1U;
    ccsds::pus::rev_a::TcHeader fitting(valid);
    return fitting.setSourceId(255U) && !fitting.setSourceId(256U)
        && ccsds_pus_identifier_fits(255U, 1U)
        && !ccsds_pus_identifier_fits(256U, 1U)
        && !ccsds_pus_identifier_fits(1U, 3U);
  });

  tester->unitTest("PUS TM decode maps every reachable C11 tailoring error.", [] {
    const auto checkA = [](const ccsds::pus::rev_a::TmTailoring &tailoring,
                           const ccsds::ErrorCode code, const char *message) {
      ccsds::pus::rev_a::TmHeader header(tailoring);
      // Deliberately malformed size: invalid tailoring must have precedence.
      const auto result = header.deserialize({});
      return !result && result.error().code() == code
          && result.error().message() == message;
    };
    const auto checkC = [](const ccsds::pus::rev_c::TmTailoring &tailoring,
                           const ccsds::ErrorCode code, const char *message) {
      ccsds::pus::rev_c::TmHeader header(tailoring);
      const auto result = header.deserialize({});
      return !result && result.error().code() == code
          && result.error().message() == message;
    };

    ccsds::pus::rev_a::TmTailoring invalidA{};
    invalidA.destinationIdOctets = 3U;
    if (!checkA(invalidA, ccsds::INVALID_SECONDARY_HEADER_DATA,
                "PUS-A TM destination-ID width must be 0, 1, 2, or 4 octets."))
      return false;

    invalidA = {};
    invalidA.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                    ccsds::time::PFieldMode::Implicit, 4U, 0U};
    if (!checkA(invalidA, ccsds::INVALID_SECONDARY_HEADER_DATA,
                "Disabled PUS-A TM time requires an empty CUC configuration."))
      return false;

    ccsds::pus::rev_c::TmTailoring invalidC{};
    invalidC.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                    ccsds::time::PFieldMode::Implicit, 4U, 0U};
    if (!checkC(invalidC, ccsds::INVALID_SECONDARY_HEADER_DATA,
                "Disabled PUS-C TM time requires an empty CUC configuration."))
      return false;

    const auto checkCuc = [&](const ccsds::time::CucConfiguration &config,
                              const char *message) {
      ccsds::pus::rev_a::TmTailoring a{};
      a.timestampPresent = true;
      a.cuc = config;
      ccsds::pus::rev_c::TmTailoring c{};
      c.timestampPresent = true;
      c.cuc = config;
      return checkA(a, ccsds::INVALID_DATA, message)
          && checkC(c, ccsds::INVALID_DATA, message);
    };
    using ccsds::time::Epoch;
    using ccsds::time::PFieldMode;
    return checkCuc({Epoch::Unspecified, PFieldMode::Implicit, 2U, 1U},
                    "CUC requires the CCSDS-1958 or an agency-defined epoch.")
        && checkCuc({Epoch::Ccsds1958Tai, static_cast<PFieldMode>(2U), 2U, 1U},
                    "CUC P-field policy is invalid.")
        && checkCuc({Epoch::Ccsds1958Tai, PFieldMode::Implicit, 0U, 1U},
                    "Basic CUC coarse time requires 1 to 4 octets.")
        && checkCuc({Epoch::Ccsds1958Tai, PFieldMode::Implicit, 5U, 1U},
                    "Basic CUC coarse time requires 1 to 4 octets.")
        && checkCuc({Epoch::Ccsds1958Tai, PFieldMode::Implicit, 2U, 4U},
                    "Basic CUC fine time requires 0 to 3 octets.");
  });

  tester->unitTest("PUS-A/C TM decoders preserve objects after invalid CUC wire data.", [] {
    const ccsds::time::CucConfiguration cuc{
      ccsds::time::Epoch::Ccsds1958Tai,
      ccsds::time::PFieldMode::Explicit, 2U, 1U
    };

    ccsds::pus::rev_a::TmTailoring tailoringA{};
    tailoringA.destinationIdOctets = 1U;
    tailoringA.packetSubcounterPresent = true;
    tailoringA.timestampPresent = true;
    tailoringA.cuc = cuc;
    ccsds::pus::rev_a::TmHeader tmA(tailoringA, 1U, 2U, 3U, 4U, {0x1234U, 0x56U});
    const auto beforeA = tmA.serialize();
    if (beforeA.size() != 9U) return false;
    auto corruptedA = beforeA;
    corruptedA[1] = 0x99U;  // Fields differ to reveal premature mutation.
    corruptedA[3] = 0x88U;
    corruptedA[4] = 0x77U;
    corruptedA[5] = 0x00U;  // Correct explicit CUC P-field is 0x15.
    const auto badA = tmA.deserialize(corruptedA);
    if (badA || badA.error().code() != ccsds::INVALID_DATA
        || badA.error().message() !=
             "CUC P-field does not match the configured epoch or widths."
        || tmA.serialize() != beforeA) return false;

    ccsds::pus::rev_c::TmTailoring tailoringC{};
    tailoringC.timestampPresent = true;
    tailoringC.cuc = cuc;
    ccsds::pus::rev_c::TmHeader tmC(
      tailoringC, 1U, 2U, 0x1234U, 0x5678U, 3U, {0x2345U, 0x67U});
    const auto beforeC = tmC.serialize();
    if (beforeC.size() != 11U) return false;
    auto corruptedC = beforeC;
    corruptedC[0] = 0x2FU;
    corruptedC[1] = 0x99U;
    corruptedC[3] = 0x88U;
    corruptedC[5] = 0x77U;
    corruptedC[7] = 0x00U;  // Invalid explicit CUC P-field.
    const auto badC = tmC.deserialize(corruptedC);
    return !badC && badC.error().code() == ccsds::INVALID_DATA
        && badC.error().message() ==
             "CUC P-field does not match the configured epoch or widths."
        && tmC.serialize() == beforeC;
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

  tester->unitTest("Configuration infers every PUS revision and direction from its selector.", [] {
    constexpr std::array<std::pair<ccsds::pus::Revision, ccsds::PacketDirection>, 4U> cases{{
      {ccsds::pus::Revision::A, ccsds::PacketDirection::Telecommand},
      {ccsds::pus::Revision::A, ccsds::PacketDirection::Telemetry},
      {ccsds::pus::Revision::C, ccsds::PacketDirection::Telecommand},
      {ccsds::pus::Revision::C, ccsds::PacketDirection::Telemetry}
    }};
    for (std::size_t index = 0U; index < cases.size(); ++index) {
      const auto [revision, direction] = cases[index];
      const auto path = "ccsdspack_pus_tailoring_" + std::to_string(index) + ".cfg";
      if (!writeTextFile(path, pusConfig(revision, direction))) return false;
      ccsds::Packet packet;
      const auto loaded = packet.loadFromConfigFile(path);
      std::remove(path.c_str());
      if (!loaded || !packet.getSecondaryHeader()
          || packet.getSecondaryHeader()->getType() != ccsds::pus::selector(revision, direction)
          || packet.getSecondaryHeader()->getDirection() != direction
          || packet.getDirection() != direction) return false;
    }
    return true;
  });

  tester->unitTest("Configuration rejects explicit CCSDS direction that contradicts PUS class identity.", [] {
    const std::string path{"ccsdspack_pus_direction_conflict.cfg"};
    auto config = pusConfig(ccsds::pus::Revision::C, ccsds::PacketDirection::Telecommand);
    config += "ccsds_type:bool=false\n";
    if (!writeTextFile(path, config)) return false;
    ccsds::Packet packet;
    const auto loaded = packet.loadFromConfigFile(path);
    std::remove(path.c_str());
    return !loaded && loaded.error().code() == ccsds::CONFIG_FILE_ERROR;
  });

  tester->unitTest("Manager decodes PUS packets using the complete Packet template tailoring.", [] {
    ccsds::pus::rev_c::TmTailoring tailoring;
    tailoring.timestampPresent = true;
    tailoring.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                     ccsds::time::PFieldMode::Explicit, 4U, 2U};
    ccsds::Packet source;
    TEST_VOID(source.setPrimaryHeader(primary(ccsds::PacketDirection::Telemetry)));
    TEST_VOID(source.setSecondaryHeader(std::make_shared<ccsds::pus::rev_c::TmHeader>(
      tailoring, 3U, 25U, 4U, 0x1234U, 2U,
      ccsds::time::CucTime{0x01020304U, 0xA0B0U})));
    TEST_VOID(source.setApplicationData({0xAAU, 0x55U}));
    const auto bytes = source.serialize();
    if (!bytes) return false;

    ccsds::Manager manager;
    TEST_VOID(manager.setPacketTemplate(source));
    TEST_VOID(manager.load(bytes.value()));
    const auto packets = manager.getPackets();
    if (packets.size() != 1U) return false;
    const auto header = std::static_pointer_cast<const ccsds::pus::rev_c::TmHeader>(
      packets.front().getSecondaryHeader());
    return header->getTimestamp() == ccsds::time::CucTime{0x01020304U, 0xA0B0U}
           && ccsds::pus::sameTailoring(
             *static_cast<const ccsds::pus::SecondaryHeader *>(source.getSecondaryHeader().get()),
             *static_cast<const ccsds::pus::SecondaryHeader *>(packets.front().getSecondaryHeader().get()));
  });
}
