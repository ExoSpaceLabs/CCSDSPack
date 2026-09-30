// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSValidator.h"
#include "CCSDSUtils.h"
#include "PusSecondaryHeaders.h"
#include <array>

namespace {
  ccsds_primary_header_t toCoreHeader(const ccsds::Header &header) noexcept {
    return {
      header.getVersionNumber(),
      header.getType(),
      header.getSecondaryHeaderFlag(),
      header.getAPID(),
      header.getSequenceFlags(),
      header.getSequenceCount(),
      header.getDataLength()
    };
  }

  std::uint8_t toCoreDirection(const ccsds::PacketDirection direction) noexcept {
    switch (direction) {
      case ccsds::PacketDirection::Telemetry:
        return CCSDS_PACKET_DIRECTION_TELEMETRY;
      case ccsds::PacketDirection::Telecommand:
        return CCSDS_PACKET_DIRECTION_TELECOMMAND;
      case ccsds::PacketDirection::Unspecified:
        return CCSDS_PACKET_DIRECTION_UNSPECIFIED;
      default:
        return 0xFFU;
    }
  }

  ccsds_cuc_config_t toCoreCuc(const ccsds::time::CucConfiguration &cuc) noexcept {
    return {
      static_cast<ccsds_cuc_epoch_t>(cuc.epoch),
      static_cast<ccsds_cuc_pfield_mode_t>(cuc.pField),
      cuc.coarseOctets,
      cuc.fineOctets
    };
  }

  bool coreTimestampValid(const ccsds::time::CucTime &timestamp,
                          const ccsds::time::CucConfiguration &cuc) noexcept {
    const auto config = toCoreCuc(cuc);
    const ccsds_cuc_time_t value{timestamp.coarse, timestamp.fine};
    std::array<std::uint8_t, 8U> bytes{};
    std::size_t written = 0U;
    return ccsds_cuc_encode(
      &value, &config, bytes.data(), bytes.size(), &written) == CCSDS_STATUS_OK;
  }

  bool coreTailoringValid(const ccsds::pus::SecondaryHeader &header) noexcept {
    if (header.getDirection() == ccsds::PacketDirection::Telecommand) {
      const auto &tc = static_cast<const ccsds::pus::TcSecondaryHeader &>(header);
      if (header.getRevision() == ccsds::pus::Revision::A) {
        const ccsds_pus_a_tc_tailoring_t tailoring{
          tc.getSourceIdOctets(), header.getSecondaryHeaderSpareOctets()
        };
        return ccsds_pus_a_tc_validate_tailoring(&tailoring) == CCSDS_STATUS_OK;
      }
      if (header.getRevision() == ccsds::pus::Revision::C) {
        return tc.getSourceIdOctets() == 2U;
      }
      return false;
    }

    if (header.getDirection() == ccsds::PacketDirection::Telemetry) {
      const auto &tm = static_cast<const ccsds::pus::TmSecondaryHeader &>(header);
      if (header.getRevision() == ccsds::pus::Revision::A) {
        const auto &aTm = static_cast<const ccsds::pus::rev_a::TmHeader &>(header);
        const ccsds_pus_a_tm_tailoring_t tailoring{
          tm.getDestinationIdOctets(),
          static_cast<std::uint8_t>(aTm.packetSubcounterPresent() ? 1U : 0U),
          static_cast<std::uint8_t>(tm.timestampPresent() ? 1U : 0U),
          toCoreCuc(tm.getCucConfiguration()),
          header.getSecondaryHeaderSpareOctets()
        };
        return ccsds_pus_a_tm_validate_tailoring(&tailoring) == CCSDS_STATUS_OK;
      }
      if (header.getRevision() == ccsds::pus::Revision::C) {
        if (tm.getDestinationIdOctets() != 2U) return false;
        const ccsds_pus_c_tm_tailoring_t tailoring{
          static_cast<std::uint8_t>(tm.timestampPresent() ? 1U : 0U),
          toCoreCuc(tm.getCucConfiguration()),
          header.getSecondaryHeaderSpareOctets()
        };
        return ccsds_pus_c_tm_validate_tailoring(&tailoring) == CCSDS_STATUS_OK;
      }
    }

    return false;
  }

  bool sameSecondaryContract(const ccsds::Packet &lhs, const ccsds::Packet &rhs) noexcept {
    const auto lhsHeader = lhs.getSecondaryHeader();
    const auto rhsHeader = rhs.getSecondaryHeader();
    if (static_cast<bool>(lhsHeader) != static_cast<bool>(rhsHeader)) return false;
    if (!lhsHeader) return true;
    if (lhsHeader->getType() != rhsHeader->getType()
        || lhsHeader->getDirection() != rhsHeader->getDirection()
        || lhsHeader->isPusHeader() != rhsHeader->isPusHeader()) return false;
    if (!lhsHeader->isPusHeader()) return true;
    return ccsds::pus::sameTailoring(
      static_cast<const ccsds::pus::SecondaryHeader &>(*lhsHeader),
      static_cast<const ccsds::pus::SecondaryHeader &>(*rhsHeader));
  }
}

const char *ccsds::validationCodeName(const ValidationCode code) noexcept {
  return ccsds_validation_code_name(static_cast<ccsds_validation_code_t>(code));
}

void ccsds::Validator::configure(const bool validatePacketCoherence,
                                 const bool validateSequenceCount,
                                 const bool validateAgainstTemplate) {
  m_validatePacketCoherence = validatePacketCoherence;
  m_validateSequenceCount = validateSequenceCount;
  m_validateAgainstTemplate = validateAgainstTemplate;
}

void ccsds::Validator::acceptSequence(const Header &header) noexcept {
  const auto coreHeader = toCoreHeader(header);
  (void)ccsds_sequence_validator_accept(&m_sequenceState, &coreHeader);
}

ccsds::ValidationReport ccsds::Validator::validate(const Packet &packet) {
  m_report = {};
  const auto &header = packet.getPrimaryHeader();
  const auto coreHeader = toCoreHeader(header);
  const bool primaryHeaderValid =
    header.getHeaderStatus() != INVALID
    && ccsds_primary_header_validate(&coreHeader) == CCSDS_STATUS_OK;

  if (!m_validatePacketCoherence) {
    setCheck(ValidationCode::PrimaryHeader, primaryHeaderValid);
    if (!primaryHeaderValid) return m_report;
  } else {
    const auto secondary = packet.getSecondaryHeader();
    bool crcValid{true};
    if (packet.getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
      auto crcInput = header.serialize();
      const auto dataFieldBytes = packet.getFullDataFieldBytes();
      crcInput.insert(crcInput.end(), dataFieldBytes.begin(), dataFieldBytes.end());
      const CRC16Config crcConfig{};
      const auto calculatedCRC = ccsds::crc16(
        crcInput, crcConfig.polynomial, crcConfig.initialValue, crcConfig.finalXorValue);
      crcValid = calculatedCRC == packet.getCRC();
    }

    const ccsds_packet_coherence_input_t input{
      coreHeader,
      packet.getSerializedSize(),
      static_cast<std::uint8_t>(header.getHeaderStatus() != INVALID),
      static_cast<std::uint8_t>(static_cast<bool>(secondary)),
      secondary ? toCoreDirection(secondary->getDirection())
                : static_cast<std::uint8_t>(CCSDS_PACKET_DIRECTION_UNSPECIFIED),
      static_cast<std::uint8_t>(
        packet.getPacketErrorControlMode() == PacketErrorControlMode::CRC16),
      static_cast<std::uint8_t>(crcValid)
    };
    ccsds_validation_report_t coreReport{};
    ccsds_validation_report_reset(&coreReport);
    const auto coreStatus = ccsds_validate_packet_coherence(
      &input, &m_sequenceState, m_validateSequenceCount ? 1 : 0, &coreReport);
    if (coreStatus != CCSDS_STATUS_OK) {
      setCheck(ValidationCode::PrimaryHeader, false);
      return m_report;
    }
    for (std::size_t i = 0U; i < coreReport.size; ++i) {
      setCheck(
        static_cast<ValidationCode>(coreReport.checks[i].code),
        coreReport.checks[i].passed != 0U);
    }
    if (!m_report.passed(ValidationCode::PrimaryHeader)) return m_report;

    if (secondary && secondary->isPusHeader()) {
      const auto &pusHeader = static_cast<const pus::SecondaryHeader &>(*secondary);
      const auto bytes = secondary->serialize();
      ccsds_pus_coherence_input_t pusInput{
        static_cast<std::uint8_t>(pusHeader.getRevision()),
        toCoreDirection(pusHeader.getDirection()),
        header.getType(),
        static_cast<std::uint8_t>(coreTailoringValid(pusHeader)),
        {
          bytes.empty() ? nullptr : bytes.data(),
          bytes.size()
        },
        secondary->getSize(),
        pusHeader.getSecondaryHeaderSpareOctets(),
        0U,
        0U,
        0U,
        0U,
        0U,
        1U,
        0U,
        0U,
        0U
      };

      if (pusHeader.getDirection() == PacketDirection::Telecommand) {
        const auto &tc = static_cast<const pus::TcSecondaryHeader &>(pusHeader);
        pusInput.acknowledgement_flags = tc.getAcknowledgementFlags();
        pusInput.identifier_octets = tc.getSourceIdOctets();
        pusInput.identifier_value = tc.getSourceId();
      } else if (pusHeader.getDirection() == PacketDirection::Telemetry) {
        const auto &tm = static_cast<const pus::TmSecondaryHeader &>(pusHeader);
        pusInput.identifier_octets = tm.getDestinationIdOctets();
        pusInput.identifier_value = tm.getDestinationId();
        pusInput.timestamp_present =
          static_cast<std::uint8_t>(tm.timestampPresent() ? 1U : 0U);
        pusInput.timestamp_valid =
          static_cast<std::uint8_t>(
            tm.timestampPresent()
            && coreTimestampValid(tm.getTimestamp(), tm.getCucConfiguration()));
        pusInput.timestamp_zero_when_absent =
          static_cast<std::uint8_t>(tm.getTimestamp() == time::CucTime{});

        if (pusHeader.getRevision() == pus::Revision::A) {
          const auto &aTm = static_cast<const pus::rev_a::TmHeader &>(pusHeader);
          pusInput.packet_subcounter_present =
            static_cast<std::uint8_t>(aTm.packetSubcounterPresent() ? 1U : 0U);
          pusInput.packet_subcounter = aTm.getPacketSubcounter();
        } else if (pusHeader.getRevision() == pus::Revision::C) {
          const auto &cTm = static_cast<const pus::rev_c::TmHeader &>(pusHeader);
          pusInput.time_reference_status = cTm.getTimeReferenceStatus();
        }
      }

      ccsds_validation_report_t pusReport{};
      ccsds_validation_report_reset(&pusReport);
      const auto pusStatus = ccsds_validate_pus_coherence(&pusInput, &pusReport);
      if (pusStatus != CCSDS_STATUS_OK) {
        setCheck(ValidationCode::PusHeader, false);
        return m_report;
      }
      for (std::size_t i = 0U; i < pusReport.size; ++i) {
        setCheck(
          static_cast<ValidationCode>(pusReport.checks[i].code),
          pusReport.checks[i].passed != 0U);
      }
    }
  }

  if (m_validateAgainstTemplate) {
    const auto &templateHeader = m_templatePacket.getPrimaryHeader();
    const ccsds_template_coherence_input_t templateInput{
      toCoreHeader(templateHeader),
      static_cast<std::uint8_t>(templateHeader.getHeaderStatus() != INVALID),
      static_cast<std::uint8_t>(
        m_templatePacket.getPacketErrorControlMode() == packet.getPacketErrorControlMode()),
      static_cast<std::uint8_t>(sameSecondaryContract(m_templatePacket, packet))
    };
    ccsds_validation_report_t coreReport{};
    ccsds_validation_report_reset(&coreReport);
    const auto coreStatus = ccsds_validate_template_coherence(
      &coreHeader, &templateInput, &coreReport);
    if (coreStatus != CCSDS_STATUS_OK) {
      setCheck(ValidationCode::PacketIdentifier, false);
      return m_report;
    }
    for (std::size_t i = 0U; i < coreReport.size; ++i) {
      setCheck(
        static_cast<ValidationCode>(coreReport.checks[i].code),
        coreReport.checks[i].passed != 0U);
    }
  }

  const bool sequenceFlagsValid =
    !m_validatePacketCoherence
    || m_report.passed(ValidationCode::SequenceFlags);
  const bool sequenceCountValid =
    !m_validatePacketCoherence
    || !m_validateSequenceCount
    || m_report.passed(ValidationCode::SequenceCount);

  if (m_validatePacketCoherence && sequenceFlagsValid
      && sequenceCountValid && m_report.valid()) {
    acceptSequence(header);
  }
  return m_report;
}

void ccsds::Validator::clear() {
  ccsds_sequence_validator_reset(&m_sequenceState);
  m_report = {};
  m_templatePacket = {};
  m_templatePacket.setUpdatePacketEnable(false);
}
