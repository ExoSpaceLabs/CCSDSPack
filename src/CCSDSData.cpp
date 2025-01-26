

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

    const auto& dataFieldHeader = getDataFieldHeader();
    if (dataFieldHeader.size() + m_applicationData.size() > m_dataPacketSize) { // check if given header exeeds header size.
        std::cout << "[ CCSDS Data ] Provided data: " << dataFieldHeader.size() + m_applicationData.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ CCSDS Data ] Error: Data field exceeds expected size.");
    }
    std::vector<uint8_t> fullData{};
    if(!dataFieldHeader.empty()) {
        fullData.insert(fullData.end(),dataFieldHeader.begin(),dataFieldHeader.end());
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

    const auto dataFieldHeader = getDataFieldHeader();
    const uint16_t maxSize = (m_applicationData.size() > dataFieldHeader.size()) ? m_applicationData.size() : dataFieldHeader.size();

    std::cout << std::endl;
    std::cout << " [CCSDS DATA] Test result:" << std::endl;
    std::cout << " [CCSDS DATA] Secondary Header Present       : [ " << (getDataFieldHeaderFlag()  ? "True" : "False") << " ]" << std::endl;
    std::cout << " [CCSDS DATA] Secondary Header         [Hex] : [ " << getBitsSpaces((maxSize - static_cast<uint16_t>(dataFieldHeader.size()))*4) ;
    for(const auto& data : dataFieldHeader) {
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
 * @brief Updates the data field header based on the current application data size.
 *
 * Updates the length field in the secondary header to match the size of the
 * application data. Ensures the header reflects the most recent data state.
 *
 * @return None.
 */
void CCSDS::DataField::updateDataFieldHeader() {
    if (!m_dataFieldHeaderUpdated) {
        if (m_dataFieldHeaderType != OTHER && m_dataFieldHeaderType != NA) {
            m_pusHeaderData->setDataLength(m_applicationData.size());;
        }
        m_dataFieldHeaderUpdated = true;
    }
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
void CCSDS::DataField::setApplicationData(const uint8_t * pData, const size_t &sizeData) {
    const auto& dataFieldHeader = getDataFieldHeader();
    if (!pData) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Data is nullptr");
    }
    if (sizeData < 1) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Data size cannot be < 1");
    }
    if (sizeData > m_dataPacketSize - dataFieldHeader.size()) {
        std::cout << "[ CCSDS Data ] Header size: " << dataFieldHeader.size() <<", data size: " << m_applicationData.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ CCSDS Data ] Error: Data field exceeds expected size.");
    }
    if (!m_applicationData.empty()) {
        std::cerr << "[ CCSDS Data ] Warning: Data field is not empty, it has been overwritten." << std::endl;
    }
    m_applicationData.assign(pData, pData + sizeData);
    updateDataFieldHeader();
}

/**
 * @brief Sets the application data using a vector of bytes.
 *
 * Replaces the current application data with the given vector and updates the header.
 *
 * @param applicationData A vector containing the application data bytes.
 *
 * @return None.
 */
void CCSDS::DataField::setApplicationData(const std::vector<uint8_t>& applicationData ) {
    m_applicationData = applicationData;
    updateDataFieldHeader();
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
void CCSDS::DataField::setDataFieldHeader(const uint8_t * pData, const size_t &sizeData) {
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
    m_dataFieldHeaderType = OTHER;
}

/**
 * @brief Sets the secondary header for the data field using a PUS Type.
 *
 * Validates and assigns the given header data to the secondary header field.
 * Ensures the header size is within acceptable limits and does not exceed
 * the remaining packet size after accounting for the application data.
 *
 * @param pData A pointer to the header data.
 * @param sizeData The size of the header data in bytes.
 * @param pType enum of type PUSType to select
 *
 * @throws std::invalid_argument If the header is null, size is zero, or exceeds the allowed size.
 * @return none.
 */
void CCSDS::DataField::setDataFieldHeader(const uint8_t * pData, const size_t &sizeData, const PUSType &pType) {
    if (!pData) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Header is nullptr");
    }
    if (!m_dataFieldHeader.empty()) {
        std::cerr <<  "[ CCSDS Data ] Warning: Secondary Header field is not empty, it has been overwritten." << std::endl;
        m_dataFieldHeader.clear();
    }
    if (pType != OTHER && pType != NA) {
        std::vector<uint8_t> data;
        data.assign(pData, pData + sizeData);
        setDataFieldHeader(data,pType);
    }else if (pType == OTHER) {
        setDataFieldHeader(pData, sizeData);
        m_dataFieldHeaderType = pType;
    }else {
        throw std::invalid_argument("[ CCSDS Data ] Error: Header type is NA (NOT APPLICABLE)");
    }
}

void CCSDS::DataField::setDataFieldHeader(const std::vector<uint8_t> &data , const PUSType &pType) {
    m_dataFieldHeaderType = pType;
    if (m_dataPacketSize < data.size() + m_applicationData.size()) {
        std::cerr << "[ PUS ] Header size: " << data.size() <<", data size: " << m_applicationData.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ PUS ] Error: Header inclusion exceeds max allowed size.");
    }
    switch (pType) {
        case PUS_A:
            if (data.size() == 6) {
                m_pusHeaderData = std::make_unique<PusA>(data);
            }else {
                throw std::invalid_argument("[ PUS ] Error: PUS-A Header field exceeds expected size.");
            }
        break;
        case PUS_B:
            if (data.size()  == 8) {
                m_pusHeaderData = std::make_unique<PusB>(data);
            }else {
                throw std::invalid_argument("[ PUS ] Error: PUS-B Header field exceeds expected size.");
            }
        break;
        case PUS_C:
            if (data.size()  == 8) {
                m_pusHeaderData = std::make_unique<PusC>(data);
            }else {
                throw std::invalid_argument("[ PUS ] Error: PUS-C Header field exceeds expected size.");
            }
        break;
        case OTHER:
            setDataFieldHeader(data);
            break;
        default: ;
    }
}


void CCSDS::DataField::setDataFieldHeader( const std::vector<uint8_t>& dataFieldHeader ) {
    if (!m_dataFieldHeader.empty()) {
        std::cerr <<  "[ CCSDS Data ] Warning: Secondary Header field is not empty, it has been overwritten." << std::endl;
        m_dataFieldHeader.clear();
    }
    m_dataFieldHeader = dataFieldHeader;
    m_dataFieldHeaderType = OTHER;
}

/**
 * @brief Sets the secondary header for the data field using a PUS-A header.
 *
 * @param header A PusA object containing the header data.
 * @return None.
 */
void CCSDS::DataField::setDataFieldHeader(const PusA& header ) {
    m_dataFieldHeaderType = PUS_A;
    m_pusHeaderData = std::make_shared<PusA>(header);
}

/**
 * @brief Sets the secondary header for the data field using a PUS-B header.
 *
 * @param header A PusB object containing the header data.
 * @return None.
 */
void CCSDS::DataField::setDataFieldHeader(const PusB& header ) {
    m_dataFieldHeaderType = PUS_B;
    m_pusHeaderData = std::make_shared<PusB>(header);
}

/**
 * @brief Sets the secondary header for the data field using a PUS-C header.
 *
 * @param header A PusC object containing the header data.
 * @return None.
 */
void CCSDS::DataField::setDataFieldHeader(const PusC& header ) {
    m_dataFieldHeaderType = PUS_C;
    m_pusHeaderData = std::make_shared<PusC>(header);
}

/**
 * @brief Retrieves the secondary header data as a vector of bytes.
 *
 * If the header type is not OTHER or NA, retrieves the data from the
 * corresponding PUS header object.
 *
 * @return A vector containing the header data bytes.
 */
std::vector<uint8_t> CCSDS::DataField::getDataFieldHeader() {
    if (m_dataFieldHeaderType != OTHER && m_dataFieldHeaderType != NA) {
        return m_pusHeaderData->serialize();;
    }
    return m_dataFieldHeader;
}
