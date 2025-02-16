

#include "CCSDSUtils.h"
#include <iostream>
#include <iomanip>

//###########################################################################
#define VERBOSE 1


std::string getBinaryString(const uint32_t value, const int bits) {
    std::string binaryString;
        // Calculate the minimum number of bits required to represent in groups of 4
    const int paddedBits = ((bits + 3) / 4) * 4;  // Round up to the nearest multiple of 4

    for (int i = paddedBits - 1; i >= 0; --i) {
        binaryString += ((value >> i) & 1) ? '1' : '0';

        // Add a space after every 4 bits, except at the end
        if (i % 4 == 0 && i != 0) {
            binaryString += ' ';
        }
    }
    return binaryString;
}

std::string getBitsSpaces(const int num){
    std::string spaces;
    
    for (int i = num - 1; i >= 0; --i) {
        spaces += ' ';
    }

    return spaces;
}

void printBufferData(const std::vector<uint8_t>& buffer) {
    std::cout << "[ ";
    for (const unsigned char i : buffer) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(i) << " ";
    }
    std::cout << "]" << std::endl;
}

/**
 * @brief Prints the data field details, including the secondary header and application data.
 *
 * Outputs information about the presence of a secondary header and the content
 * of both the secondary header and the application data in hexadecimal format.
 *
 * @return none.
 */
void printData(CCSDS::DataField dataField) {
    const auto dataFieldHeader = dataField.getDataFieldHeader();
    auto applicationData = dataField.getApplicationData();
    const uint16_t maxSize = (applicationData.size() > dataFieldHeader.size()) ? applicationData.size() : dataFieldHeader.size();

    std::cout << std::endl;
    std::cout << " [CCSDS DATA] Test result:" << std::endl;
    std::cout << " [CCSDS DATA] Secondary Header Present       : [ " << (dataField.getDataFieldHeaderFlag()  ? "True" : "False") << " ]" << std::endl;
    std::cout << " [CCSDS DATA] Secondary Header         [Hex] : [ " << getBitsSpaces((maxSize - static_cast<uint16_t>(dataFieldHeader.size()))*4) ;
    for(const auto& data : dataFieldHeader) {
        std::cout << "0x" << std::hex << static_cast<unsigned int>(data) << " ";
    }

    std::cout <<"]" <<  std::endl;
    std::cout << " [CCSDS DATA] Data Field               [Hex] : [ " << getBitsSpaces((maxSize - static_cast<uint16_t>(applicationData.size()))*4) ;
    for(const auto& data : applicationData) {
        std::cout << "0x" << std::hex << static_cast<unsigned int>(data)<< " ";
    }
    std::cout <<"]" <<  std::endl;
    std::cout << std::endl;
}


/**
 * @brief Prints the header fields and their binary or hexadecimal representations.
 *
 * Outputs all relevant header fields, including the full primary header, version number,
 * type, data field header flag, APID, sequence flags, sequence count, and data length.
 * Each field is displayed with appropriate formatting and spacing.
 *
 * @return none.
 */
void printHeader(CCSDS::Header &header) {

    std::cout << std::endl;
    std::cout << " [CCSDS HEADER] Test result:" << std::endl;
    std::cout << " [CCSDS HEADER] Full Primary Header    [Hex] : [ " << getBitsSpaces(17-12) << "0x" << std::hex << header.getFullHeader() << " ]" << std::endl;
    std::cout << std::endl;
    std::cout << " [CCSDS HEADER] Info: Version Number         : [ " << getBitsSpaces(19- 4) << getBinaryString(       header.getVersionNumber(),  3 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Type                   : [ " << getBitsSpaces(19- 4) << getBinaryString(                header.getType(),  1 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Data Field Header Flag : [ " << getBitsSpaces(19- 4) << getBinaryString( header.getDataFieldHeaderFlag(),  1 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: APID                   : [ " << getBitsSpaces(17-12) << getBinaryString(                header.getAPID(), 11 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Sequence Flags         : [ " << getBitsSpaces(19- 4) << getBinaryString(       header.getSequenceFlags(),  2 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Sequence Count         : [ " << getBitsSpaces(    0) << getBinaryString(       header.getSequenceCount(), 14 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: DataLength             : [ "                              << getBinaryString(          header.getDataLength(), 16 ) << " ]" << std::endl;
    std::cout << std::endl;
}


void printPrimaryHeader(CCSDS::Packet& packet) {
    CCSDS::Header header{packet.getPrimaryHeader()};
    printHeader(header);
}

/**
 * @brief Prints the data field and the CRC-16 checksum of the packet.
 *
 * Outputs the content of the data field and the CRC-16 checksum
 * in hexadecimal format to the standard output.
 *
 * @return none.
 */
void printDataField(CCSDS::Packet& packet) {

    CCSDS::DataField dataField{};
    ASSIGN_OR_PRINT(dataField, packet.getDataField());

    printData(dataField);
    std::cout << "[ CCSDSPack ] CRC-16                   [Hex] : [ "<< "0x" << std::hex << packet.getCRC() << " ]" << std::endl;
}

uint16_t crc16(
    const std::vector<uint8_t>& data, const uint16_t polynomial, const uint16_t initialValue, const uint16_t finalXorValue) {
    uint16_t crc = initialValue;

    for (const auto& byte : data) {
        crc ^= static_cast<uint16_t>(byte) << 8;  // Align byte with MSB of 16-bit CRC
        for (int i = 0; i < 8; ++i) {             // Process each bit
            if (crc & 0x8000) {                   // Check if MSB is set
                crc = (crc << 1) ^ polynomial;    // Shift and XOR with polynomial
            } else {
                crc = crc << 1;                   // Shift only
            }
        }
    }

    return crc ^ finalXorValue; // Apply final XOR
}


/**
 * @brief Marks the start of a unit test.
 *
 * Initializes the timer and tracks whether this is the first test in a series.
 */
void TestManager::unitTestStart() {
    if (!m_testStarted) {
        m_testStarted = true;
        m_startTime = std::chrono::high_resolution_clock::now();
        m_unitStartTime = m_startTime;
    }else {
        m_unitStartTime = std::chrono::high_resolution_clock::now();
    }
}

/**
 * @brief Marks the end of a unit test and logs its result.
 *
 * @param condition The result of the test (true for pass, false for fail).
 * @param message A message describing the test.
 */
void TestManager::unitTestEnd(const bool condition, const std::string& message) {
    m_unitEndTime = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = m_unitEndTime - m_unitStartTime;
    const auto us = elapsed.count() * 1000000;
    m_totalTime += us;
    m_testCounter++;
    if (condition) {
        m_testsPassed++;
#if VERBOSE == 1
        std::cout << "# "<< std::setw(3) << std::setfill(' ') << m_testCounter
        << " [ "<< GREEN << "PASS" << RESET << " | "<< std::setw(7) << std::setfill(' ') << us
        << " us ] Test: "<<  message << std::endl;
#endif
    }else {
        m_testsFailed++;
#if VERBOSE == 1
        std::cout << "# "<< std::setw(3) << std::setfill(' ') << m_testCounter
        << " [ " << RED << "FAIL" << RESET<< " | " <<  std::setw(7) << std::setfill(' ') << us
        << " us ] Test: "<<  message << std::endl;
#endif
    }
}

int TestManager::Result() {
    constexpr int spaceSize = 7;
    m_testStarted = false;
    const std::chrono::duration<double> elapsed = m_unitEndTime - m_startTime;
    const auto us = elapsed.count() * 1000000;

    std::cout <<"Test Results:" << std::endl;
    std::cout << "   PASSED:    [" << std::setw(spaceSize + 4) << std::setfill(' ') << m_testsPassed << " |"
              << std::setw(spaceSize + 4) << std::setfill(' ') << m_testsFailed
              << " ] :FAILED,     TOTAL: [" << std::setw(spaceSize) << std::setfill(' ') << m_testCounter << " ]\n"
              << "   TEST TIME: [ " << std::setw(spaceSize) << std::setfill(' ') << us << " us | "
              << std::setw(spaceSize) << std::setfill(' ') << m_totalTime << " us ] :ACTUAL TIME";
    std::cout << std::endl;
    return static_cast<int>(m_testsFailed);
}

void printPackets(std::vector<CCSDS::Packet>& packets ) {
    int idx = 0;
    for (auto& packet : packets) {
        std::cout << "[ CCSDS Manager ] Printing Packet [ "<< idx << " ]:" << std::endl;
        std::cout << "[ CCSDS Manager ] Data ";
        printBufferData(packet.serialize());

        printPrimaryHeader(packet);
        printDataField(packet);

        idx++;
    }
}