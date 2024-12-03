

#include "CCSDSData.h"
#include "CCSDSUtils.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

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
