
#include "CCSDSValidator.h"
#include <CCSDSUtils.h>

void CCSDS::Validator::configure(const bool validatePacketCoherence, const bool validateAgainstTemplate) {
    m_validatePacketCoherence = validatePacketCoherence;
    m_validateAgainstTemplate = validateAgainstTemplate;
}

bool CCSDS::Validator::validate(const Packet &packet) {
    m_report.clear();
    m_report.reserve(m_reportSize);
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
        m_report[1] = crc16(dataFieldBytes) == toValidate.getCRC();
        result &= m_report[1];
        if (toValidateHeader.getSequenceFlags() == UNSEGMENTED) {
            m_report[2] = toValidateHeader.getSequenceCount() == 0;
        }
        else {
            m_report[2] = toValidateHeader.getSequenceCount() > 0;
        }
        result &= m_report[2];
    }

    m_templatePacket.setUpdatePacketEnable(false);
    auto templateHeader = m_templatePacket.getPrimaryHeader();
    const auto templateHeaderData = templateHeader.serialize();

    if (m_validateAgainstTemplate) {
        m_report[3] = templateHeaderData[0] == toValidateHeaderData[0] && templateHeaderData[1] == toValidateHeaderData[1];
        result &= m_report[3];
        m_report[4] = templateHeader.getDataFieldHeaderFlag() == toValidate.getDataFieldHeaderFlag();
        result &= m_report[4];
    }
    return result;
}
