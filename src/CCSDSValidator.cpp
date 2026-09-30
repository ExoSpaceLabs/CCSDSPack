// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSValidator.h"
#include "CCSDSUtils.h"
#include "PusSecondaryHeaders.h"

namespace {
  bool identifierFits(const std::uint32_t value, const std::uint8_t octets) noexcept {
    if (octets == 0U) return value == 0U;
    if (octets >= 4U) return true;
    return value < (1UL << (octets * 8U));
  }

  bool validRevision(const ccsds::pus::Revision revision) noexcept {
    return revision == ccsds::pus::Revision::A || revision == ccsds::pus::Revision::C;
  }

  bool validDirection(const ccsds::PacketDirection direction) noexcept {
    return direction == ccsds::PacketDirection::Telecommand
           || direction == ccsds::PacketDirection::Telemetry;
  }

  bool reservedBitsValid(const std::vector<std::uint8_t> &bytes,
                         const ccsds::pus::Revision revision,
                         const ccsds::PacketDirection direction) noexcept {
    if (bytes.empty()) return false;
    if (revision == ccsds::pus::Revision::A
        && direction == ccsds::PacketDirection::Telecommand) {
      return (bytes[0] & 0x80U) == 0U && ((bytes[0] >> 4U) & 0x07U) == 1U;
    }
    if (revision == ccsds::pus::Revision::A
        && direction == ccsds::PacketDirection::Telemetry) {
      return bytes[0] == 0x10U;
    }
    if (revision == ccsds::pus::Revision::C) return (bytes[0] >> 4U) == 2U;
    return false;
  }

  bool spareFieldsValid(const std::vector<std::uint8_t> &bytes,
                        const std::uint8_t spareOctets) noexcept {
    const auto spare = static_cast<std::size_t>(spareOctets);
    if (spare > bytes.size()) return false;
    for (std::size_t i = bytes.size() - spare; i < bytes.size(); ++i)
      if (bytes[i] != 0U) return false;
    return true;
  }

  bool cucIsEmpty(const ccsds::time::CucConfiguration &cuc) noexcept {
    return cuc.epoch == ccsds::time::Epoch::Unspecified
           && cuc.pField == ccsds::time::PFieldMode::Implicit
           && cuc.coarseOctets == 0U && cuc.fineOctets == 0U;
  }

  bool pusTailoringValid(const ccsds::pus::SecondaryHeader &header) {
    if (!validRevision(header.getRevision()) || !validDirection(header.getDirection())) return false;
    if (header.getDirection() == ccsds::PacketDirection::Telecommand) {
      const auto &tc = static_cast<const ccsds::pus::TcSecondaryHeader &>(header);
      if (!ccsds::pus::validIdentifierWidth(tc.getSourceIdOctets())) return false;
      return header.getRevision() != ccsds::pus::Revision::C || tc.getSourceIdOctets() == 2U;
    }
    const auto &tm = static_cast<const ccsds::pus::TmSecondaryHeader &>(header);
    if (!ccsds::pus::validIdentifierWidth(tm.getDestinationIdOctets())) return false;
    if (header.getRevision() == ccsds::pus::Revision::C && tm.getDestinationIdOctets() != 2U)
      return false;
    if (tm.timestampPresent()) return static_cast<bool>(ccsds::time::validate(tm.getCucConfiguration()));
    return cucIsEmpty(tm.getCucConfiguration());
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
  const ccsds_primary_header_t coreHeader{
    header.getVersionNumber(),
    header.getType(),
    header.getSecondaryHeaderFlag(),
    header.getAPID(),
    header.getSequenceFlags(),
    header.getSequenceCount(),
    header.getDataLength()
  };
  (void)ccsds_sequence_validator_accept(&m_sequenceState, &coreHeader);
}

ccsds::ValidationReport ccsds::Validator::validate(const Packet &packet) {
  m_report = {};
  const auto &header = packet.getPrimaryHeader();
  const auto headerData = header.serialize();
  const bool primaryHeaderValid = header.getHeaderStatus() != INVALID && headerData.size() == 6U;
  setCheck(ValidationCode::PrimaryHeader, primaryHeaderValid);
  if (!primaryHeaderValid) return m_report;

  bool sequenceFlagsValid{true};
  bool sequenceCountValid{true};

  if (m_validatePacketCoherence) {
    setCheck(ValidationCode::PacketVersion, header.getVersionNumber() == 0U);
    const auto serializedSize = packet.getSerializedSize();
    const auto packetDataFieldSize = serializedSize >= 6U ? serializedSize - 6U : 0U;
    setCheck(ValidationCode::PacketDataLength,
             packetDataFieldSize > 0U && header.getDataLength() == packetDataFieldSize - 1U);

    if (packet.getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
      auto crcInput = headerData;
      const auto dataFieldBytes = packet.getFullDataFieldBytes();
      crcInput.insert(crcInput.end(), dataFieldBytes.begin(), dataFieldBytes.end());
      const CRC16Config crcConfig{};
      const auto calculatedCRC = ccsds::crc16(
        crcInput, crcConfig.polynomial, crcConfig.initialValue, crcConfig.finalXorValue);
      setCheck(ValidationCode::Crc16, calculatedCRC == packet.getCRC());
    }

    const auto secondary = packet.getSecondaryHeader();
    const bool secondaryPresence =
      (header.getSecondaryHeaderFlag() != 0U) == static_cast<bool>(secondary);
    setCheck(ValidationCode::SecondaryHeaderPresence, secondaryPresence);

    if (secondary && secondary->getDirection() != PacketDirection::Unspecified) {
      setCheck(ValidationCode::SecondaryHeaderDirection,
               header.getType() == packetTypeForDirection(secondary->getDirection()));
    }

    if (secondary && secondary->isPusHeader()) {
      setCheck(ValidationCode::PusHeader, true);
      const auto &pusHeader = static_cast<const pus::SecondaryHeader &>(*secondary);
      const bool revisionValid = validRevision(pusHeader.getRevision());
      const bool directionValid = validDirection(pusHeader.getDirection());
      setCheck(ValidationCode::PusRevision, revisionValid);
      setCheck(ValidationCode::PusDirection, directionValid);
      setCheck(ValidationCode::PusPacketType,
               directionValid
               && header.getType() == packetTypeForDirection(pusHeader.getDirection()));
      setCheck(ValidationCode::PusTailoring, pusTailoringValid(pusHeader));

      const auto bytes = secondary->serialize();
      const bool sizeValid = !bytes.empty() && bytes.size() == secondary->getSize();
      setCheck(ValidationCode::PusSecondaryHeaderSize, sizeValid);
      setCheck(ValidationCode::PusReservedBits,
               sizeValid && reservedBitsValid(bytes, pusHeader.getRevision(), pusHeader.getDirection()));
      setCheck(ValidationCode::PusSpareFields,
               sizeValid && spareFieldsValid(bytes, pusHeader.getSecondaryHeaderSpareOctets()));

      if (pusHeader.getDirection() == PacketDirection::Telecommand) {
        const auto &tc = static_cast<const pus::TcSecondaryHeader &>(pusHeader);
        setCheck(ValidationCode::PusAcknowledgement, tc.getAcknowledgementFlags() <= 0x0FU);
        setCheck(ValidationCode::PusSourceId,
                 identifierFits(tc.getSourceId(), tc.getSourceIdOctets()));
      } else if (pusHeader.getDirection() == PacketDirection::Telemetry) {
        const auto &tm = static_cast<const pus::TmSecondaryHeader &>(pusHeader);
        setCheck(ValidationCode::PusDestinationId,
                 identifierFits(tm.getDestinationId(), tm.getDestinationIdOctets()));
        if (tm.timestampPresent()) {
          const auto timestamp = time::serialize(tm.getTimestamp(), tm.getCucConfiguration());
          setCheck(ValidationCode::PusTimestamp, static_cast<bool>(timestamp));
        } else {
          setCheck(ValidationCode::PusTimestamp, tm.getTimestamp() == time::CucTime{});
        }
        if (pusHeader.getRevision() == pus::Revision::A) {
          const auto &aTm = static_cast<const pus::rev_a::TmHeader &>(pusHeader);
          setCheck(ValidationCode::PusPacketSubcounter,
                   aTm.packetSubcounterPresent() || aTm.getPacketSubcounter() == 0U);
        } else if (pusHeader.getRevision() == pus::Revision::C) {
          const auto &cTm = static_cast<const pus::rev_c::TmHeader &>(pusHeader);
          setCheck(ValidationCode::PusTimeReferenceStatus,
                   cTm.getTimeReferenceStatus() <= 0x0FU);
        }
      }
    }

    sequenceFlagsValid = ccsds_sequence_flags_valid(
      &m_sequenceState, header.getSequenceFlags()) != 0;
    setCheck(ValidationCode::SequenceFlags, sequenceFlagsValid);
    if (m_validateSequenceCount) {
      sequenceCountValid = ccsds_sequence_count_valid(
        &m_sequenceState, header.getSequenceCount()) != 0;
      setCheck(ValidationCode::SequenceCount, sequenceCountValid);
    }
  }

  if (m_validateAgainstTemplate) {
    const auto &templateHeader = m_templatePacket.getPrimaryHeader();
    const auto templateHeaderData = templateHeader.serialize();
    const bool templateHeaderValid = templateHeader.getHeaderStatus() != INVALID
                                     && templateHeaderData.size() == 6U;
    setCheck(ValidationCode::PacketIdentifier,
             templateHeaderValid && templateHeaderData[0] == headerData[0]
             && templateHeaderData[1] == headerData[1]);
    bool segmentationClassValid{false};
    if (templateHeaderValid) {
      segmentationClassValid = templateHeader.getSequenceFlags() == UNSEGMENTED
        ? header.getSequenceFlags() == UNSEGMENTED
        : header.getSequenceFlags() != UNSEGMENTED;
    }
    setCheck(ValidationCode::SegmentationClass, segmentationClassValid);
    setCheck(ValidationCode::TemplatePacketErrorControl,
             m_templatePacket.getPacketErrorControlMode() == packet.getPacketErrorControlMode());
    setCheck(ValidationCode::TemplateSecondaryHeader,
             sameSecondaryContract(m_templatePacket, packet));
  }

  if (m_validatePacketCoherence && sequenceFlagsValid
      && (!m_validateSequenceCount || sequenceCountValid) && m_report.valid()) {
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
