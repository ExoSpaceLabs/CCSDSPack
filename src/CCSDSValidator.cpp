// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSValidator.h"
#include "CCSDSUtils.h"

void CCSDS::Validator::configure(const bool validatePacketCoherence,
                                 const bool validateSequenceCount,
                                 const bool validateAgainstTemplate) {
  m_validatePacketCoherence = validatePacketCoherence;
  m_validateSegmentedCount = validateSequenceCount;
  m_validateAgainstTemplate = validateAgainstTemplate;
}

void CCSDS::Validator::acceptSequence(const Header &header) {
  const auto next = static_cast<std::uint16_t>(
    (header.getSequenceCount() + 1U) & SEQUENCE_COUNT_MASK);
  auto state = static_cast<std::uint16_t>(SEQUENCE_INITIALIZED_MASK | next);
  if (header.getSequenceFlags() == FIRST_SEGMENT
      || header.getSequenceFlags() == CONTINUING_SEGMENT) {
    state |= SEGMENT_OPEN_MASK;
  }
  m_sequenceCounter = state;
}

bool CCSDS::Validator::validate(const Packet &packet) {
  m_report.assign(m_reportSize, true);
  bool result{true};

  const auto &header = packet.getPrimaryHeader();
  const auto headerData = header.serialize();
  if (headerData.size() != 6U) {
    m_report.assign(m_reportSize, false);
    return false;
  }
  const auto dataFieldBytes = packet.getFullDataFieldBytes();

  if (m_validatePacketCoherence) {
    const auto packetDataFieldSize =
      dataFieldBytes.size() + packet.getPacketErrorControlSize();
    m_report[0] = packetDataFieldSize > 0U
                  && header.getDataLength() == packetDataFieldSize - 1U;
    result &= m_report[0];

    if (packet.getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
      auto crcInput = headerData;
      crcInput.insert(crcInput.end(), dataFieldBytes.begin(), dataFieldBytes.end());
      const auto calculatedCRC = ::crc16(
        crcInput, m_CRCConfig.polynomial, m_CRCConfig.initialValue,
        m_CRCConfig.finalXorValue);
      m_report[1] = calculatedCRC == packet.getCRC();
    }
    result &= m_report[1];

    const auto flags = static_cast<ESequenceFlag>(header.getSequenceFlags());
    const auto open = segmentOpen();
    switch (flags) {
      case UNSEGMENTED:
      case FIRST_SEGMENT:
        m_report[2] = !open;
        break;
      case CONTINUING_SEGMENT:
      case LAST_SEGMENT:
        m_report[2] = open;
        break;
      default:
        m_report[2] = false;
        break;
    }
    result &= m_report[2];

    if (m_validateSegmentedCount && sequenceInitialized()) {
      m_report[3] = header.getSequenceCount() == expectedSequenceCount();
    }
    result &= m_report[3];

    if (m_report[2] && m_report[3]) {
      acceptSequence(header);
    }
  }

  if (m_validateAgainstTemplate) {
    const auto &templateHeader = m_templatePacket.getPrimaryHeader();
    const auto templateHeaderData = templateHeader.serialize();
    if (templateHeaderData.size() != 6U) {
      m_report[4] = false;
      m_report[5] = false;
      return false;
    }

    m_report[4] = templateHeaderData[0] == headerData[0]
                  && templateHeaderData[1] == headerData[1];
    result &= m_report[4];

    if (templateHeader.getSequenceFlags() == UNSEGMENTED) {
      m_report[5] = header.getSequenceFlags() == UNSEGMENTED;
    } else {
      m_report[5] = header.getSequenceFlags() != UNSEGMENTED;
    }
    result &= m_report[5];
  }

  return result;
}

void CCSDS::Validator::clear() {
  m_sequenceCounter = 0U;
  m_report.clear();
  m_templatePacket = {};
  m_templatePacket.setUpdatePacketEnable(false);
}
