

#include "CCSDSPack.h"
#include <iostream>

void CCSDS::Packet::printDataField() {
    m_dataField.printData();
    std::cout << "[ CCSDSPack ] CRC-16                   [Hex] : [ "<< "0x" << std::hex << getCRC() << " ]" << std::endl;
}

void CCSDS::Packet::calculateCRC16() {
    constexpr uint16_t POLYNOMIAL      = 0x1021; // CCSDS CRC-16 polynomial (x^16 + x^12 + x^5 + 1)
    constexpr uint16_t INITIAL_VALUE   = 0xFFFF; // Initial value
    constexpr uint16_t FINAL_XOR_VALUE = 0x0000; // No final XOR in CCSDS

    uint16_t crc = INITIAL_VALUE;

    for (const auto& byte : m_dataField.getFullDataField()) {
        crc ^= static_cast<uint16_t>(byte) << 8;  // Align byte with MSB of 16-bit CRC
        for (int i = 0; i < 8; ++i) {             // Process each bit
            if (crc & 0x8000) {                   // Check if MSB is set
                crc = (crc << 1) ^ POLYNOMIAL;    // Shift and XOR with polynomial
            } else {
                crc = crc << 1;                   // Shift only
            }
        }
    }
    m_crcCalculated = true;
    m_CRC16 =  crc ^ FINAL_XOR_VALUE;             // Apply final XOR (if needed)
}

uint16_t CCSDS::Packet::getCRC() {
    if (!m_crcCalculated) {
        calculateCRC16();
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
    auto dataField = m_dataField.getFullDataField();
    const auto crc = getCRCVector();
    auto header = getPrimaryHeaderVector();

    packet.insert(packet.end(), header.begin(), header.end());
    packet.insert(packet.end(), dataField.begin(), dataField.end());
    packet.insert(packet.end(), crc.begin(), crc.end());

    return packet;
}

