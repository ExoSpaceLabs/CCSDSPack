
#include "PusServices.h"


CCSDS::ResultBool PusA::deserialize(const std::vector<uint8_t> &data) {
  RET_IF_ERR_MSG(data.size() != m_size, CCSDS::ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A header not correct size (size != 6 bytes)");

  m_version = data[0] & 0x5;
  m_serviceType = data[1];
  m_serviceSubType = data[2];
  m_sourceID = data[3];
  m_dataLength = data[4] << 8 | data[5];
  return true;
}

std::vector<uint8_t> PusA::serialize() const {
  std::vector data{
    static_cast<uint8_t>(m_version & 0x7),
    m_serviceType,
    m_serviceSubType,
    m_sourceID,
    static_cast<uint8_t>(m_dataLength >> 8 & 0xFF),
    static_cast<uint8_t>(m_dataLength & 0xFF),
  };

  return data;
}

CCSDS::ResultBool PusB::deserialize(const std::vector<uint8_t> &data) {
  RET_IF_ERR_MSG(data.size() != m_size, CCSDS::ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-B header not correct size (size != 8 bytes)");
  m_version = data[0] & 0x7;
  m_serviceType = data[1];
  m_serviceSubType = data[2];
  m_sourceID = data[3];
  m_eventID = data[4] << 8 | data[5];
  m_dataLength = data[6] << 8 | data[7];
  return true;
}

std::vector<uint8_t> PusB::serialize() const {
  std::vector data{
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

CCSDS::ResultBool PusC::deserialize(const std::vector<uint8_t> &data) {
  RET_IF_ERR_MSG(data.size() != m_size, CCSDS::ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C header not correct size (size != 8 bytes)");

  m_version = data[0] & 0x7;
  m_serviceType = data[1];
  m_serviceSubType = data[2];
  m_sourceID = data[3];
  m_timeCode = data[4] << 8 | data[5];
  m_dataLength = data[6] << 8 | data[7];
  return true;
}

std::vector<uint8_t> PusC::serialize() const {
  std::vector data{
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
