//
// Created by dev on 1/18/25.
//

#include "PUSService.h"
#include <vector>
#include <cstdint>


std::vector<uint8_t> CCSDS::PusA::getData() const {
  std::vector<uint8_t> data{
    m_version,
    m_serviceType,
    m_serviceSubType,
    m_sourceID,
    static_cast<uint8_t>(m_timeStamp >> 24 & 0xFF),
    static_cast<uint8_t>(m_timeStamp >> 16 & 0xFF),
    static_cast<uint8_t>(m_timeStamp >> 8 & 0xFF),
    static_cast<uint8_t>(m_timeStamp & 0xFF),
  };

  return data;
}

std::vector<uint8_t> CCSDS::PusB::getData() const {
  std::vector<uint8_t> data{
    m_version,
    m_serviceType,
    m_serviceSubType,
    m_destinationID,
    static_cast<uint8_t>(m_sequenceControl >> 8 & 0xFF),
    static_cast<uint8_t>(m_sequenceControl & 0xFF),
  };

  return data;
}

std::vector<uint8_t> CCSDS::PusC::getData() {
  std::vector<uint8_t> data{
    m_version,
    m_serviceType,
    m_serviceSubType,
    m_sourceID,
  };
  data.insert(data.end(), m_missionData.begin(), m_missionData.end());

  return data;
}