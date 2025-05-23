#include "CCSDSValidator.h"
#include <CCSDSUtils.h>

void CCSDS::Validator::configure(const bool validatePacketCoherence, bool validateSequenceCount, const bool validateAgainstTemplate) {
  m_validatePacketCoherence = validatePacketCoherence;
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
  // auto coherence checks
  const auto dataFieldBytes = toValidate.getFullDataFieldBytes();
  const auto dataFieldBytesSize = dataFieldBytes.size();

  // test CRC therefore full data field coherence
  if (m_validatePacketCoherence) {
    m_report[0] = toValidateHeader.getDataLength() == dataFieldBytesSize;
    result &= m_report[0];
    const auto calcCRC = crc16(dataFieldBytes);
    m_report[1] = calcCRC == toValidate.getCRC();
    result &= m_report[1];
    if (toValidateHeader.getSequenceFlags() == UNSEGMENTED) {
      m_report[2] = toValidateHeader.getSequenceCount() == 0;
    } else {
      m_report[2] = toValidateHeader.getSequenceCount() > 0;
      if (m_validateSegmentedCount) {
        if (toValidateHeader.getSequenceFlags() == FIRST_SEGMENT) {
          m_report[3] = toValidateHeader.getSequenceCount() == 1;
        }else {
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
    m_report[4] = templateHeaderData[0] == toValidateHeaderData[0] && templateHeaderData[1] == toValidateHeaderData[1];
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
