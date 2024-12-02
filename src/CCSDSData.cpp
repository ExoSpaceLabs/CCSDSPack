

#include "CCSDSData.h"
#include "CCSDSUtils.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

std::vector<uint8_t> CCSDS::DataField::getFullDataField() {
    if (m_secondaryHeader.size() + m_data.size() > m_dataPacketSize) { // check if given header exeeds header size.
        std::cout << "[ CCSDS Data ] Provided data: " << m_secondaryHeader.size() + m_data.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ CCSDS Data ] Error: Data field exceeds expected size.");
    }
    std::vector<uint8_t> fullData{};
    if(!m_secondaryHeader.empty()) {
        fullData.insert(fullData.end(),m_secondaryHeader.begin(),m_secondaryHeader.end());
    }
    fullData.insert(fullData.end(),m_data.begin(),m_data.end());
    return fullData;
}
void CCSDS::DataField::printData() {

    const uint16_t maxSize = (m_data.size() > m_secondaryHeader.size()) ? m_data.size() : m_secondaryHeader.size();

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

void CCSDS::DataField::setData(const uint8_t * pData, const size_t sizeData) {
    if (!pData) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Data is nullptr");
    }
    if (sizeData < 1) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Data size cannot be < 1");
    }
    if (sizeData > m_dataPacketSize - m_secondaryHeader.size()) {
        std::cout << "[ CCSDS Data ] Header size: " << m_secondaryHeader.size() <<", data size: " << m_data.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ CCSDS Data ] Error: Data field exceeds expected size.");
    }
    if (!m_data.empty()) {
        std::cerr << "[ CCSDS Data ] Warning: Data field is not empty, it has been overwritten." << std::endl;
    }
    m_data.assign(pData, pData + sizeData);
}

void CCSDS::DataField::setSecondaryHeader(const uint8_t * pData,  const size_t sizeData) {
    if (!pData) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Header is nullptr");
    }
    if (sizeData < 1) {
        throw std::invalid_argument("[ CCSDS Data ] Error: Header size cannot be < 1");
    }
    if (sizeData > m_dataPacketSize - m_data.size()) {
        std::cout << "[ CCSDS Data ] Header size: " << m_secondaryHeader.size() <<", data size: " << m_data.size() << ", max size: "<< m_dataPacketSize << std::endl;
        throw std::invalid_argument("[ CCSDS Data ] Error: Header field exceeds expected size.");
    }
    if (!m_secondaryHeader.empty()) {
        std::cerr <<  "[ CCSDS Data ] Warning: Secondary Header field is not empty, it has been overwritten." << std::endl;
    }
    m_secondaryHeader.assign(pData, pData + sizeData);
}
