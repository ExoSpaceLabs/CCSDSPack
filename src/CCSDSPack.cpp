

#include "CCSDSPack.h"

#include <CCSDSUtils.h>
#include <iostream>

void CCSDS::Packet::printDataField() {
    m_dataField.printData();
    std::cout << "[ CCSDSPack ] CRC-16                   [Hex] : [ "<< "0x" << std::hex << getCRC() << " ]" << std::endl;
}

uint16_t CCSDS::Packet::getCRC() {
    if (!m_crcCalculated) {
        m_CRC16 = crc16(m_dataField.getFullDataField());
        m_crcCalculated = true;
    }
    return m_CRC16;
}

std::vector<uint8_t> CCSDS::Packet::getCRCVector() {
    std::vector<uint8_t> crc(2);
    const auto crcVar = getCRC();
    crc[0] = (crcVar >> 8) & 0xFF; // MSB (Most Significant Byte)
    crc[1] = crcVar & 0xFF;        // LSB (Least Significant Byte)
    return crc;
}

std::vector<uint8_t> CCSDS::Packet::getPrimaryHeaderVector() {
    std::vector<uint8_t> header(6);
    const auto headerVar = getPrimaryHeader();
    for (int i = 0; i < 6; ++i) {
        header[i] = (headerVar >> (40 - i * 8)) & 0xFF; // Extract MSB to LSB
    }

    return header;
}

std::vector<uint8_t> CCSDS::Packet::getFullPacket() {

    std::vector<uint8_t> packet{};
    auto header         =       getPrimaryHeaderVector();
    auto dataField = m_dataField.getFullDataField();
    //ToDo Check if header and data are assigned, throw error if not.  header.size = 6, data.size > 1
    const auto crc      =                 getCRCVector();


    packet.insert(packet.end(),    header.begin(),    header.end());
    packet.insert(packet.end(), dataField.begin(), dataField.end());
    packet.insert(packet.end(),       crc.begin(),       crc.end());

    return packet;
}

