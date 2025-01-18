

#include "CCSDSPack.h"

#include <CCSDSUtils.h>
#include <iostream>

/**
 * @brief Prints the data field and the CRC-16 checksum of the packet.
 *
 * Outputs the content of the data field and the CRC-16 checksum
 * in hexadecimal format to the standard output.
 *
 * @return none.
 */
void CCSDS::Packet::printDataField() {
    m_dataField.printData();
    std::cout << "[ CCSDSPack ] CRC-16                   [Hex] : [ "<< "0x" << std::hex << getCRC() << " ]" << std::endl;
}

/**
 * @brief Computes and retrieves the CRC-16 checksum of the packet.
 *
 * If the CRC-16 has not already been calculated, this function computes it
 * using the full data field of the packet. The result is cached for
 * future calls to improve performance.
 *
 * @return The 16-bit CRC-16 checksum.
 */
uint16_t CCSDS::Packet::getCRC() {
    if (!m_crcCalculated) {
        m_CRC16 = crc16(m_dataField.getFullDataField());
        m_crcCalculated = true;
    }
    return m_CRC16;
}

/**
 * @brief Retrieves the CRC-16 checksum as a vector of bytes.
 *
 * The checksum is split into its most significant byte (MSB) and
 * least significant byte (LSB) and stored in a two-element vector.
 *
 * @return A vector containing the MSB and LSB of the CRC-16 checksum.
 */
std::vector<uint8_t> CCSDS::Packet::getCRCVector() {
    std::vector<uint8_t> crc(2);
    const auto crcVar = getCRC();
    crc[0] = (crcVar >> 8) & 0xFF; // MSB (Most Significant Byte)
    crc[1] = crcVar & 0xFF;        // LSB (Least Significant Byte)
    return crc;
}

/**
 * @brief Retrieves the primary header of the packet as a vector of bytes.
 *
 * Extracts the 48-bit primary header and converts it into a six-element
 * vector, ordered from the most significant byte (MSB) to the least
 * significant byte (LSB).
 *
 * @return A vector containing the six bytes of the primary header.
 */

std::vector<uint8_t> CCSDS::Packet::getPrimaryHeaderVector() {
    std::vector<uint8_t> header(6);
    const auto headerVar = getPrimaryHeader();
    for (int i = 0; i < 6; ++i) {
        header[i] = (headerVar >> (40 - i * 8)) & 0xFF; // Extract MSB to LSB
    }

    return header;
}

/**
 * @brief Retrieves the full packet as a vector of bytes.
 *
 * Combines the primary header, data field, and CRC-16 checksum into
 * a single vector. Ensures that the header and data field sizes meet
 * minimum requirements.
 *
 * @note Header size must be 6 bytes and data field size must be greater than 1 byte.
 *
 * @return A vector containing the full packet in byte form.
 */
std::vector<uint8_t> CCSDS::Packet::getFullPacket() {

    auto header         =       getPrimaryHeaderVector();
    auto dataField = m_dataField.getFullDataField();
    //ToDo Check if header and data are assigned, throw error if not.  header.size = 6, data.size > 1
    const auto crc      =                 getCRCVector();

    std::vector<uint8_t> packet{};
    packet.insert(packet.end(),    header.begin(),    header.end());
    packet.insert(packet.end(), dataField.begin(), dataField.end());
    packet.insert(packet.end(),       crc.begin(),       crc.end());

    return packet;
}

