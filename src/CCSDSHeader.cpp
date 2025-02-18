
#include "CCSDSHeader.h"

#include "CCSDSUtils.h"
#include <stdexcept>


void CCSDS::Header::deserialize(const std::vector<uint8_t>& data) {
    //todo return ResultBool
    // use set data to handle the hard work
    if (data.size() != 6) {
        throw std::invalid_argument("Invalid Header Data");
    }
    uint64_t headerData = 0;
    for (int i = 0; i < 6; ++i) {
        headerData |= static_cast<uint64_t>(data[i]) << (40 - i * 8); // Combine MSB to LSB
    }
    setData(headerData);
}

/**
 * @brief Sets the header data from a 64-bit integer representation.
 *
 * Decomposes the 64-bit input data into various header fields, including the version number,
 * type, data field header flag, APID, sequence flags, sequence count, and data length.
 *
 * @param data The 64-bit integer representing the header data.
 * @throws std::invalid_argument If the input data exceeds the valid bit range for the header.
 * @return none.
 */
void CCSDS::Header::setData(const uint64_t &data){
  //todo return ResultBool
    if (data > 0xFFFFFFFFFFFF) { // check if given header exeeds header size.
        throw std::invalid_argument("[ CCSDS Header ] Error: Input data exceeds expected bit size for version or size.");
    }
    // Decompose data using mask and shifts
    m_dataLength                     = (data & 0xFFFF);               // last 16 bits
    m_packetSequenceControl          = (data >> 16) & 0xFFFF;         // middle 16 bits
    m_packetIdentificationAndVersion = (data >> 32);                  // first 16 bits

    // decompose packet identifier
    m_versionNumber       = (m_packetIdentificationAndVersion >> 13);          // First 3 bits for version
    m_type                = (m_packetIdentificationAndVersion >> 12) & 0x1;    // Next 1 bit
    m_dataFieldHeaderFlag = (m_packetIdentificationAndVersion >> 11) & 0x1;    // Next 1 bit
    m_APID                = (m_packetIdentificationAndVersion & 0x07FF);       // Last 11 bits

    // decompose sequence control
    m_sequenceFlags = (m_packetSequenceControl >> 14);                // first 2 bits
    m_sequenceCount = (m_packetSequenceControl & 0x3FFF);             // Last 14 bits.

}

CCSDS::Header::Header(const std::vector<uint8_t>& data) {
  //todo return ResultBool impossible therefore just print.
    deserialize(data);
}

std::vector<uint8_t> CCSDS::Header::serialize()  {

    m_packetSequenceControl = (static_cast<uint16_t>(m_sequenceFlags) << 14) | m_sequenceCount;
    m_packetIdentificationAndVersion = (static_cast<uint16_t>(m_versionNumber) << 13) | (m_type << 12) | static_cast<uint16_t>((m_dataFieldHeaderFlag) << 11) | m_APID;

    std::vector data{
        static_cast<unsigned char>(m_packetIdentificationAndVersion >> 8),
        static_cast<unsigned char>(m_packetIdentificationAndVersion & 0xFF),
        static_cast<unsigned char>(m_packetSequenceControl >> 8),
        static_cast<unsigned char>(m_packetSequenceControl & 0xFF),
        static_cast<unsigned char>(m_dataLength >> 8),
        static_cast<unsigned char>(m_dataLength & 0xFF),
    };
    return data;
}

/**
 * @brief Sets the header data from a `PrimaryHeader` structure.
 *
 * Assigns values from a `PrimaryHeader` structure to the internal header fields.
 * Combines certain fields into their packed representations for efficient storage.
 *
 * @param data A `PrimaryHeader` structure containing the header data.
 * @return none.
 */
void CCSDS::Header::setData(const PrimaryHeader &data){

    m_versionNumber       =       data.versionNumber & 0x0007;
    m_type                =                data.type & 0x0001;
    m_dataFieldHeaderFlag = data.dataFieldHeaderFlag & 0x0001;
    m_APID                =                data.APID & 0x07FF;
    m_sequenceFlags       =       data.sequenceFlags & 0x0003;
    m_sequenceCount       =       data.sequenceCount & 0x3FFF;
    m_dataLength          =          data.dataLength;

    m_packetSequenceControl = (static_cast<uint16_t>(m_sequenceFlags) << 14) | m_sequenceCount;
    m_packetIdentificationAndVersion = (static_cast<uint16_t>(m_versionNumber) << 13) | (m_type << 12) | static_cast<uint16_t>((m_dataFieldHeaderFlag) << 11) | m_APID;
}

