

#include "CCSDSPack.h"
#include <iostream>


// ToDo
// these functions are utilities. not related to the project top me moved
std::string getBinaryString(uint32_t value, int bits) {
    std::string binaryString;
        // Calculate the minimum number of bits required to represent in groups of 4
    int paddedBits = ((bits + 3) / 4) * 4;  // Round up to the nearest multiple of 4

    for (int i = paddedBits - 1; i >= 0; --i) {
        binaryString += ((value >> i) & 1) ? '1' : '0';

        // Add a space after every 4 bits, except at the end
        if (i % 4 == 0 && i != 0) {
            binaryString += ' ';
        }
    }
    return binaryString;
}

std::string getBitsSpaces(int num){
    std::string spaces;
    
    for (int i = num - 1; i >= 0; --i) {
        spaces += ' ';
    }

    return spaces;
}


void CCSDSHeader::setData(uint64_t data){
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

void CCSDSHeader::setData(PrimaryHeader data){

    m_versionNumber       = data.versionNumber & 0x0007;
    m_type                = data.type & 0x0001;
    m_dataFieldHeaderFlag = data.dataFieldHeaderFlag & 0x0001;
    m_APID                = data.APID & 0x07FF;
    m_sequenceFlags       = data.sequenceFlags & 0x0003;
    m_sequenceCount       = data.sequenceCount & 0x3FFF;
    m_dataLength          = data.dataLength;

    m_packetSequenceControl = (static_cast<uint16_t>(m_sequenceFlags) << 14) | m_sequenceCount;
    m_packetIdentificationAndVersion = (static_cast<uint16_t>(m_versionNumber) << 13) | (m_type << 12) | static_cast<uint16_t>((m_dataFieldHeaderFlag) << 11) | m_APID;
}



void CCSDSHeader::printHeader(){

    std::cout << std::endl;
    std::cout << " [CCSDS HEADER] Test result:" << std::endl;
    std::cout << " [CCSDS HEADER] Full Primary Header    [Hex] : [ " << getBitsSpaces(17-12) << "0x" << std::hex << getFullHeader() << " ]" << std::endl;
    std::cout << std::endl;
    std::cout << " [CCSDS HEADER] Info: Version Number         : [ " << getBitsSpaces(19- 4) << getBinaryString(       getVersionNumber(),  3 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Type                   : [ " << getBitsSpaces(19- 4) << getBinaryString(                getType(),  1 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Data Field Header Flag : [ " << getBitsSpaces(19- 4) << getBinaryString( getDataFieldheaderFlag(),  1 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: APID                   : [ " << getBitsSpaces(17-12) << getBinaryString(                getAPID(), 11 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Sequence Flags         : [ " << getBitsSpaces(19- 4) << getBinaryString(       getSequenceFlags(),  2 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Sequence Count         : [ " << getBitsSpaces(16-16) << getBinaryString(       getSequenceCount(), 14 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: DataLength             : [ "                         << getBinaryString(          getDataLength(), 16 ) << " ]" << std::endl;
    std::cout << std::endl;
}

std::vector<uint8_t> CCSDSDataField::getFullDataField() {
    if (m_secondaryHeader.size() + m_data.size() > m_dataPacketSize) { // check if given header exeeds header size.
        std::cout << "[ CCSDS Data ] Provided data: " << m_secondaryHeader.size() + m_data.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ CCSDS Data ] Error: Data field is bigger than expected size");
    }
    std::vector<uint8_t> fullData{};
    if(!m_secondaryHeader.empty()) {
        fullData.insert(fullData.end(),m_secondaryHeader.begin(),m_secondaryHeader.end());
    }
    fullData.insert(fullData.end(),m_data.begin(),m_data.end());
    return fullData;
}
void CCSDSDataField::printData() {

    uint16_t maxSize = (m_data.size() > m_secondaryHeader.size()) ? m_data.size() : m_secondaryHeader.size();

    std::cout << std::endl;
    std::cout << " [CCSDS DATA] Test result:" << std::endl;
    std::cout << " [CCSDS DATA] Secondary Header Present       : [ " << ((!m_secondaryHeader.empty() ) ? "True" : "False") << " ]" << std::endl;
    std::cout << " [CCSDS DATA] Secondary Header         [Hex] : [ " << getBitsSpaces((maxSize - static_cast<uint16_t>(m_secondaryHeader.size()))*4) ;
    for(const auto& data : m_secondaryHeader) {
        std::cout << "0x" << std::hex << static_cast<unsigned int>(data) << " ";
    }

    std::cout <<"]" <<  std::endl;
    std::cout << " [CCSDS DATA] Data Field               [Hex] : [ " << getBitsSpaces((maxSize - static_cast<uint16_t>(m_data.size()))*4) ;
    for(const auto& data : m_data) {
        std::cout << "0x" << std::hex << static_cast<unsigned int>(data)<< " ";
    }
    std::cout <<"]" <<  std::endl;
    std::cout << std::endl;
}


void CCSDSPacket::printDataField() {
    m_dataField.printData();
    calculateCRC16();
    std::cout << "[ CCSDSPack ] CRC-16                   [Hex] : [ "<< "0x" << std::hex << getCRC() << " ]" << std::endl;
}


void CCSDSPacket::calculateCRC16() {
    constexpr uint16_t POLYNOMIAL = 0x1021; // CCSDS CRC-16 polynomial (x^16 + x^12 + x^5 + 1)
    constexpr uint16_t INITIAL_VALUE = 0xFFFF; // Initial value
    constexpr uint16_t FINAL_XOR_VALUE = 0x0000; // No final XOR in CCSDS

    uint16_t crc = INITIAL_VALUE;

    for (const auto& byte : m_dataField.getFullDataField()) {
        crc ^= static_cast<uint16_t>(byte) << 8; // Align byte with MSB of 16-bit CRC
        for (int i = 0; i < 8; ++i) {           // Process each bit
            if (crc & 0x8000) {                 // Check if MSB is set
                crc = (crc << 1) ^ POLYNOMIAL;  // Shift and XOR with polynomial
            } else {
                crc = crc << 1;                 // Shift only
            }
        }
    }

    m_CRC16 =  crc ^ FINAL_XOR_VALUE; // Apply final XOR (if needed)
}
