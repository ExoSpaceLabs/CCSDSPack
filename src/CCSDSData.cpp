

#include "CCSDSData.h"
#include "CCSDSUtils.h"
#include <iostream>
#include <stdexcept>

std::vector<uint8_t> CCSDS::DataField::getFullDataField() {
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
void CCSDS::DataField::printData() {

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