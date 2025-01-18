

#include "CCSDSPack.h"
#include "CCSDSData.h"

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
 * @brief Updates Primary headers data field size.
 *
 * Uses the currently set data field size to set the header.
 *
 * @return none.
 */
void CCSDS::Packet::updatePrimaryHeader() {
    if (!m_updatedHeader) {
        const auto dataFiledSize = static_cast<uint16_t>( m_dataField.getFullDataField().size());
        const auto dataFieldHeaderFlag = m_dataField.getDataFieldHeaderFlag();
        m_primaryHeader.setDataLength(dataFiledSize);
        m_primaryHeader.setDataFieldheaderFlag(dataFieldHeaderFlag);
        m_primaryHeader.setSequenceCount(m_sequenceCounter);
        m_updatedHeader = true;
    }
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

uint64_t CCSDS::Packet::getPrimaryHeader() {
    updatePrimaryHeader();
    return  m_primaryHeader.getFullHeader();
};

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
    updatePrimaryHeader();
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

void CCSDS::Packet::setPrimaryHeader( const uint64_t data ) {
    m_primaryHeader.setData( data );
    m_crcCalculated = false;
    m_updatedHeader = false;
}
void CCSDS::Packet::setPrimaryHeader( const PrimaryHeader data ) {
    m_primaryHeader.setData( data );
    m_crcCalculated = false;
    m_updatedHeader = false;
}
void CCSDS::Packet::setDataFieldHeader(const PusA header ) {
    m_dataField.setDataFieldHeader( header );
    m_crcCalculated = false;
    m_updatedHeader = false;
}
void CCSDS::Packet::setDataFieldHeader(const PusB header ) {
    m_dataField.setDataFieldHeader( header );
    m_crcCalculated = false;
    m_updatedHeader = false;
}
void CCSDS::Packet::setDataFieldHeader(const PusC &header ) {
    m_dataField.setDataFieldHeader( header );
    m_crcCalculated = false;
    m_updatedHeader = false;
}
void CCSDS::Packet::setDataFieldHeader( const std::vector<uint8_t>& data ) {
    m_dataField.setDataFieldHeader( data );
    m_crcCalculated = false;
    m_updatedHeader = false;
}
void CCSDS::Packet::setDataFieldHeader( const uint8_t* pData, const size_t sizeData ) {
    m_dataField.setDataFieldHeader( pData,sizeData );
    m_crcCalculated = false;
    m_updatedHeader = false;
}
void CCSDS::Packet::setApplicationData( const std::vector<uint8_t>& data ) {
    m_dataField.setApplicationData( data );
    m_crcCalculated = false;
    m_updatedHeader = false;
}
void CCSDS::Packet::setApplicationData( const uint8_t* pData, const size_t sizeData ) {
    m_dataField.setApplicationData( pData,sizeData );
    m_crcCalculated = false;
    m_updatedHeader = false;
}
