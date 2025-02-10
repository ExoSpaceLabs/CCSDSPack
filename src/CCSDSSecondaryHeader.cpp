//
// Created by dev on 1/18/25.
//

#include "CCSDSSecondaryHeader.h"
#include <vector>
#include <stdexcept>
#include <cstdint>

void CCSDS::SecondaryHeaderAbstract::setDataLength(const uint16_t dataLength) {
  // Default implementation (optional or could throw an exception)
  (void)dataLength; // To avoid unused parameter warning
}

void CCSDS::SecondaryHeaderAbstract::deserialize(const std::vector<uint8_t> &data) {
  (void)data;
}

uint16_t CCSDS::SecondaryHeaderAbstract::getDataLength() const {
  return 0;
}

uint8_t CCSDS::SecondaryHeaderAbstract::getSize() const {
  // Default implementation
  return 0; // or an appropriate default value
}

std::vector<uint8_t> CCSDS::SecondaryHeaderAbstract::serialize() const {
  // Default implementation
  return {}; // Return an empty vector
}

CCSDS::PusA::PusA(const std::vector<uint8_t> &data) {
  deserialize(data);
}

void CCSDS::PusA::deserialize(const std::vector<uint8_t> &data) {
  if (data.size() != m_size) {
    throw std::invalid_argument("[ PUS ] Error: PUS-A header not correct size.");
  }
  m_version = data[0] &0x5;
  m_serviceType = data[1];
  m_serviceSubType = data[2];
  m_sourceID = data[3];
  m_dataLength = data[4] << 8 | data[5];
}

std::vector<uint8_t> CCSDS::PusA::serialize() const {
  std::vector<uint8_t> data{
    static_cast<uint8_t>(m_version & 0x7),
    m_serviceType,
    m_serviceSubType,
    m_sourceID,
    static_cast<uint8_t>(m_dataLength >> 8 & 0xFF),
    static_cast<uint8_t>(m_dataLength & 0xFF),
  };

  return data;
}

CCSDS::PusB::PusB(const std::vector<uint8_t>& data) {
  deserialize(data);
}

void CCSDS::PusB::deserialize(const std::vector<uint8_t> &data) {
  if (data.size() != m_size) {
    throw std::invalid_argument("[ PUS ] Error: PUS-B header not correct size.");
  }
  m_version = data[0] &0x7;
  m_serviceType = data[1];
  m_serviceSubType = data[2];
  m_sourceID = data[3];
  m_eventID = data[4] << 8 | data[5];
  m_dataLength = data[6] << 8 | data[7];
}

std::vector<uint8_t> CCSDS::PusB::serialize() const {
  std::vector<uint8_t> data{
    static_cast<uint8_t>(m_version & 0x7),
    m_serviceType,
    m_serviceSubType,
    m_sourceID,
    static_cast<uint8_t>(m_eventID >> 8 & 0xFF),
    static_cast<uint8_t>(m_eventID & 0xFF),
    static_cast<uint8_t>(m_dataLength >> 8 & 0xFF),
    static_cast<uint8_t>(m_dataLength & 0xFF),
  };

  return data;
}

CCSDS::PusC::PusC(const std::vector<uint8_t>& data) {
  deserialize(data);
}

void CCSDS::PusC::deserialize(const std::vector<uint8_t> &data) {
  if (data.size() != m_size) {
    throw std::invalid_argument("[ PUS ] Error: PUS-C header not correct size.");
  }
  m_version = data[0] &0x7;
  m_serviceType = data[1];
  m_serviceSubType = data[2];
  m_sourceID = data[3];
  m_timeCode = data[4] << 8 | data[5];
  m_dataLength = data[6] << 8 | data[7];
}

std::vector<uint8_t> CCSDS::PusC::serialize() const {
  std::vector<uint8_t> data{
    static_cast<uint8_t>(m_version & 0x7),
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
