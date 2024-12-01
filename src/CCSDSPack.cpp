

#include "CCSDSPack.h"
#include <iostream>

void CCSDS::Packet::printDataField() {
    m_dataField.printData();
    calculateCRC16();
    std::cout << "[ CCSDSPack ] CRC-16                   [Hex] : [ "<< "0x" << std::hex << getCRC() << " ]" << std::endl;
}


void CCSDS::Packet::calculateCRC16() {
    constexpr uint16_t POLYNOMIAL = 0x1021;      // CCSDS CRC-16 polynomial (x^16 + x^12 + x^5 + 1)
    constexpr uint16_t INITIAL_VALUE = 0xFFFF;   // Initial value
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

    m_CRC16 =  crc ^ FINAL_XOR_VALUE;             // Apply final XOR (if needed)
}
