

#include "CCSDSData.h"
#include "CCSDSUtils.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

/**
 * @brief Retrieves the full data field by combining the data field header and application data.
 *
 * Combines the secondary header (if present) and application data into a single vector.
 * Ensures that the total size does not exceed the maximum allowed data packet size.
 *
 * @throws std::invalid_argument If the total size of the header and data exceeds the allowed size.
 * @return A vector containing the full data field (header + application data).
 */
std::vector<uint8_t> CCSDS::DataField::getFullDataField() {
    if (m_dataFieldHeader.size() + m_applicationData.size() > m_dataPacketSize) { // check if given header exeeds header size.
        std::cout << "[ CCSDS Data ] Provided data: " << m_dataFieldHeader.size() + m_applicationData.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ CCSDS Data ] Error: Data field exceeds expected size.");
    }
    std::vector<uint8_t> fullData{};
    if(!m_dataFieldHeader.empty()) {
        fullData.insert(fullData.end(),m_dataFieldHeader.begin(),m_dataFieldHeader.end());
    }
    fullData.insert(fullData.end(),m_applicationData.begin(),m_applicationData.end());
    return fullData;
}

/**
 * @brief Prints the data field details, including the secondary header and application data.
 *
 * Outputs information about the presence of a secondary header and the content
 * of both the secondary header and the application data in hexadecimal format.
 *
 * @return none.
 */
void CCSDS::DataField::printData() {

    const uint16_t maxSize = (m_applicationData.size() > m_dataFieldHeader.size()) ? m_applicationData.size() : m_dataFieldHeader.size();

    std::cout << std::endl;
    std::cout << " [CCSDS DATA] Test result:" << std::endl;
    std::cout << " [CCSDS DATA] Secondary Header Present       : [ " << ((!m_dataFieldHeader.empty() ) ? "True" : "False") << " ]" << std::endl;
    std::cout << " [CCSDS DATA] Secondary Header         [Hex] : [ " << getBitsSpaces((maxSize - static_cast<uint16_t>(m_dataFieldHeader.size()))*4) ;
    for(const auto& data : m_dataFieldHeader) {
        std::cout << "0x" << std::hex << static_cast<unsigned int>(data) << " ";
    }

    std::cout <<"]" <<  std::endl;
    std::cout << " [CCSDS DATA] Data Field               [Hex] : [ " << getBitsSpaces((maxSize - static_cast<uint16_t>(m_applicationData.size()))*4) ;
    for(const auto& data : m_applicationData) {
        std::cout << "0x" << std::hex << static_cast<unsigned int>(data)<< " ";
    }
    std::cout <<"]" <<  std::endl;
    std::cout << std::endl;
}

/**
 * @brief Sets the application data for the data field.
 *
 * Validates and assigns the given application data to the data field.
 * Ensures the data size is within acceptable limits and does not exceed
 * the remaining packet size after accounting for the header.
 *
 * @param pData A pointer to the application data.
 * @param sizeData The size of the application data in bytes.
 *
 * @throws std::invalid_argument If the data is null, size is zero, or exceeds the allowed size.
 * @return none.
 */
void CCSDS::DataField::setApplicationData(const uint8_t * pData, const size_t sizeData) {
    if (!pData) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Data is nullptr");
    }
    if (sizeData < 1) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Data size cannot be < 1");
    }
    if (sizeData > m_dataPacketSize - m_dataFieldHeader.size()) {
        std::cout << "[ CCSDS Data ] Header size: " << m_dataFieldHeader.size() <<", data size: " << m_applicationData.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ CCSDS Data ] Error: Data field exceeds expected size.");
    }
    if (!m_applicationData.empty()) {
        std::cerr << "[ CCSDS Data ] Warning: Data field is not empty, it has been overwritten." << std::endl;
    }
    m_applicationData.assign(pData, pData + sizeData);
}

/**
 * @brief Sets the secondary header for the data field.
 *
 * Validates and assigns the given header data to the secondary header field.
 * Ensures the header size is within acceptable limits and does not exceed
 * the remaining packet size after accounting for the application data.
 *
 * @param pData A pointer to the header data.
 * @param sizeData The size of the header data in bytes.
 *
 * @throws std::invalid_argument If the header is null, size is zero, or exceeds the allowed size.
 * @return none.
 */
void CCSDS::DataField::setDataFieldHeader(const uint8_t * pData,  const size_t sizeData) {
    if (!pData) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Header is nullptr");
    }
    if (sizeData < 1) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Header size cannot be < 1");
    }
    if (sizeData > m_dataPacketSize - m_applicationData.size()) {
        std::cout << "[ CCSDS Data ] Header size: " << m_dataFieldHeader.size() <<", data size: " << m_applicationData.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ CCSDS Data ] Error: Header field exceeds expected size.");
    }
    if (!m_dataFieldHeader.empty()) {
        std::cerr <<  "[ CCSDS Data ] Warning: Secondary Header field is not empty, it has been overwritten." << std::endl;
    }
    m_dataFieldHeader.assign(pData, pData + sizeData);
}
