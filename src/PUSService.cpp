//
// Created by dev on 1/18/25.
//

#include "PUSService.h"
#include <vector>
#include <cstdint>

void CCSDS::PusHeader::setDataLength(const uint16_t dataLength) {
  // Default implementation (optional or could throw an exception)
  (void)dataLength; // To avoid unused parameter warning
}

uint16_t CCSDS::PusHeader::getDataLength() const {
  return 0;
}

uint8_t CCSDS::PusHeader::getSize() const {
  // Default implementation
  return 0; // or an appropriate default value
}

std::vector<uint8_t> CCSDS::PusHeader::getData() const {
  // Default implementation
  return {}; // Return an empty vector
}

std::vector<uint8_t> CCSDS::PusA::getData() const {
  std::vector<uint8_t> data{
    static_cast<unsigned char>(m_version & 0x3),
    m_serviceType,
    m_serviceSubType,
    m_sourceID,
    static_cast<uint8_t>(m_dataLength >> 8 & 0xFF),
    static_cast<uint8_t>(m_dataLength & 0xFF),
  };

  return data;
}

std::vector<uint8_t> CCSDS::PusB::getData() const {
  std::vector<uint8_t> data{
    static_cast<unsigned char>(m_version & 0x3),
    m_serviceType,
    m_serviceSubType,
    m_sourceID,
    m_eventID,
    static_cast<uint8_t>(m_dataLength >> 8 & 0xFF),
    static_cast<uint8_t>(m_dataLength & 0xFF),
  };

  return data;
}

std::vector<uint8_t> CCSDS::PusC::getData() const {
  std::vector<uint8_t> data{
    static_cast<unsigned char>(m_version & 0x3),
    m_serviceType,
    m_serviceSubType,
    m_sourceID,
    static_cast<uint8_t>(m_timeCode >> 8 & 0xFF),
    static_cast<uint8_t>(m_timeCode & 0xFF),
    static_cast<uint8_t>(m_dataLength >> 8 & 0xFF),
    static_cast<uint8_t>(m_dataLength & 0xFF),
  };

  return data;
}

