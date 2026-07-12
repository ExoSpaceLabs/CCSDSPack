// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSValidator.h"
#include <CCSDSUtils.h>

void CCSDS::Validator::configure(const bool validatePacketCoherence, const bool validateSequenceCount,
                                  const bool validateAgainstTemplate) {
  m_validatePacketCoherence = validatePacketCoherence;
  m_validateSegmentedCount = validateSequenceCount;
  m_validateAgainstTemplate = validateAgainstTemplate;
}

bool CCSDS::Validator::validate(const Packet &packet) {
  m_report.clear();
  m_report.reserve(m_reportSize);
  m_report.assign({true, true, true, true, true, true});
  bool result{true};
  auto toValidate = packet;
  toValidate.setUpdatePacketEnable(false);
  auto toValidateHeader = toValidate.getPrimaryHeader();
  const auto toValidateHeaderData = toValidateHeader.serialize();
  if (toValidateHeaderData.size() != 6U) {
    m_report.assign(m_reportSize, false);
    return false;
  }
  const auto dataFieldBytes = toValidate.getFullDataFieldBytes();

  if (m_validatePacketCoherence) {
    const auto packetDataFieldSize = dataFieldBytes.size() + toValidate.getPacketErrorControlSize();
    m_report[0] = packetDataFieldSize > 0U
                  && toValidateHeader.getDataLength() == packetDataFieldSize - 1U;
    result &= m_report[0];

    if (toValidate.getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
      auto crcInput = toValidateHeaderData;
      crcInput.insert(crcInput.end(), dataFieldBytes.begin(), dataFieldBytes.end());
      const auto calculatedCRC = crc16(crcInput, m_CRCConfig.polynomial, m_CRCConfig.initialValue,
                                       m_CRCConfig.finalXorValue);
      m_report[1] = calculatedCRC == toValidate.getCRC();
    }
    result &= m_report[1];

    if (toValidateHeader.getSequenceFlags() == UNSEGMENTED) {
      m_report[2] = toValidateHeader.getSequenceCount() == 0;
    } else {
      m_report[2] = toValidateHeader.getSequenceCount() > 0;
      if (m_validateSegmentedCount) {
        if (toValidateHeader.getSequenceFlags() == FIRST_SEGMENT) {
          m_report[3] = toValidateHeader.getSequenceCount() == 1;
        } else {
          m_report[3] = toValidateHeader.getSequenceCount() == m_sequenceCounter;
        }
        m_sequenceCounter++;
      }
    }
    result &= m_report[2];
    result &= m_report[3];
  }

  if (m_validateAgainstTemplate) {
    m_templatePacket.setUpdatePacketEnable(false);
    auto templateHeader = m_templatePacket.getPrimaryHeader();
    const auto templateHeaderData = templateHeader.serialize();
    if (templateHeaderData.size() != 6U) {
      m_report[4] = false;
      m_report[5] = false;
      return false;
    }
    m_report[4] = templateHeaderData[0] == toValidateHeaderData[0]
                  && templateHeaderData[1] == toValidateHeaderData[1];
    result &= m_report[4];
    if (templateHeader.getSequenceFlags() == UNSEGMENTED) {
      m_report[5] = toValidateHeader.getSequenceFlags() == UNSEGMENTED;
    } else {
      m_report[5] = toValidateHeader.getSequenceFlags() != UNSEGMENTED;
    }
    result &= m_report[5];
  }
  return result;
}

void CCSDS::Validator::clear() {
  m_sequenceCounter = 1;
  m_report.clear();
  m_templatePacket = {};
  m_templatePacket.setUpdatePacketEnable(false);
}
