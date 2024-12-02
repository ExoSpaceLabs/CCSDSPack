

#include "CCSDSHeader.h"
#include "CCSDSUtils.h"
#include <iostream>
#include <stdexcept>

void CCSDS::Header::setData(const uint64_t data){
    if (data > 0xFFFFFFFFFFFF) { // check if given header exeeds header size.
        throw std::invalid_argument("[ CCSDS Header ] Error: Input data exceeds expected bit size for version or size.");
    }

    // Decompose data using mask and
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

void CCSDS::Header::setData(const PrimaryHeader data){

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



void CCSDS::Header::printHeader(){

    std::cout << std::endl;
    std::cout << " [CCSDS HEADER] Test result:" << std::endl;
    std::cout << " [CCSDS HEADER] Full Primary Header    [Hex] : [ " << getBitsSpaces(17-12) << "0x" << std::hex << getFullHeader() << " ]" << std::endl;
    std::cout << std::endl;
    std::cout << " [CCSDS HEADER] Info: Version Number         : [ " << getBitsSpaces(19- 4) << getBinaryString(       getVersionNumber(),  3 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Type                   : [ " << getBitsSpaces(19- 4) << getBinaryString(                getType(),  1 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Data Field Header Flag : [ " << getBitsSpaces(19- 4) << getBinaryString( getDataFieldheaderFlag(),  1 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: APID                   : [ " << getBitsSpaces(17-12) << getBinaryString(                getAPID(), 11 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Sequence Flags         : [ " << getBitsSpaces(19- 4) << getBinaryString(       getSequenceFlags(),  2 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Sequence Count         : [ " << getBitsSpaces(    0) << getBinaryString(       getSequenceCount(), 14 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: DataLength             : [ "                              << getBinaryString(          getDataLength(), 16 ) << " ]" << std::endl;
    std::cout << std::endl;
}
