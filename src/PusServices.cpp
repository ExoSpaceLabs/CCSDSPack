
#include "PusServices.h"
#include "CCSDSDataField.h"
#include "CCSDSUtils.h"

CCSDS::ResultBool PusA::deserialize(const std::vector<uint8_t> &data) {
  RET_IF_ERR_MSG(data.size() != m_size, CCSDS::ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A header not correct size (size != 6 bytes)");

  m_version = data[0] & 0x7;
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

void PusA::update(CCSDS::DataField* dataField) {
  m_dataLength = dataField->getApplicationDataBytesSize();
}

CCSDS::ResultBool PusA::loadFromConfig(const Config& cfg) {
  uint8_t version = 0;
  uint8_t serviceType = 0;
  uint8_t serviceSubType = 0;
  uint8_t sourceId = 0;

  RET_IF_ERR_MSG(!cfg.isKey("pus_version"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_version");
  RET_IF_ERR_MSG(!cfg.isKey("pus_service_type"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_service_type");
  RET_IF_ERR_MSG(!cfg.isKey("pus_service_sub_type"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_service_sub_type");
  RET_IF_ERR_MSG(!cfg.isKey("pus_source_id"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_source_id");

  ASSIGN_OR_PRINT(version, cfg.get<int>("pus_version"));
  ASSIGN_OR_PRINT(serviceType, cfg.get<int>("pus_service_type"));
  ASSIGN_OR_PRINT(serviceSubType,cfg.get< int>("pus_service_sub_type"));
  ASSIGN_OR_PRINT(sourceId,cfg.get<int>("pus_source_id"));

  m_version = version & 0x7;
  m_serviceType = serviceType & 0xFF;
  m_serviceSubType = serviceSubType & 0xFF;
  m_sourceID = sourceId & 0xFF;

  return true;
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

void PusB::update(CCSDS::DataField* dataField) {
  m_dataLength = dataField->getApplicationDataBytesSize();
}

CCSDS::ResultBool PusB::loadFromConfig(const Config &cfg) {
  uint8_t version = 0;
  uint8_t serviceType = 0;
  uint8_t serviceSubType = 0;
  uint8_t sourceId = 0;
  uint8_t eventId = 0;

  RET_IF_ERR_MSG(!cfg.isKey("pus_version"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_version");
  RET_IF_ERR_MSG(!cfg.isKey("pus_service_type"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_service_type");
  RET_IF_ERR_MSG(!cfg.isKey("pus_service_sub_type"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_service_sub_type");
  RET_IF_ERR_MSG(!cfg.isKey("pus_source_id"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_source_id");
  RET_IF_ERR_MSG(!cfg.isKey("pus_event_id"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_event_id");

  ASSIGN_OR_PRINT(version, cfg.get<int>("pus_version"));
  ASSIGN_OR_PRINT(serviceType, cfg.get<int>("pus_service_type"));
  ASSIGN_OR_PRINT(serviceSubType,cfg.get< int>("pus_service_sub_type"));
  ASSIGN_OR_PRINT(sourceId,cfg.get<int>("pus_source_id"));
  ASSIGN_OR_PRINT(eventId,cfg.get<int>("pus_event_id"));

  m_version = version & 0x7;
  m_serviceType = serviceType & 0xFF;
  m_serviceSubType = serviceSubType & 0xFF;
  m_sourceID = sourceId & 0xFF;
  m_eventID = eventId & 0xFFFF;

  return true;
}

CCSDS::ResultBool PusC::deserialize(const std::vector<uint8_t> &data) {
  RET_IF_ERR_MSG(data.size() <= m_size, CCSDS::ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C header not correct size (size <= 6 bytes)");
  m_version = data[0] & 0x7;
  m_serviceType = data[1];
  m_serviceSubType = data[2];
  m_sourceID = data[3];
  m_dataLength = data[data.size()-2] << 8 | data[data.size()-1];
  if (data.size() > 6) {
    m_timeCode.assign(data.begin() + 4, data.end()-2);
  }
  return true;
}

std::vector<uint8_t> PusC::serialize() const {
  std::vector data{
    static_cast<uint8_t>(m_version & 0x7),
    m_serviceType,
    m_serviceSubType,
    m_sourceID
  };
  if (!m_timeCode.empty()) {
    data.insert(data.end(), m_timeCode.begin(), m_timeCode.end());
  }
  data.push_back(m_dataLength >> 8 & 0xFF);
  data.push_back(m_dataLength & 0xFF);

  return data;
}

void PusC::update(CCSDS::DataField* dataField) {
  m_dataLength = dataField->getApplicationDataBytesSize();
}

CCSDS::ResultBool PusC::loadFromConfig(const Config& cfg) {
  uint8_t version = 0;
  uint8_t serviceType = 0;
  uint8_t serviceSubType = 0;
  uint8_t sourceId = 0;
  std::vector<uint8_t> timeCode{};

  RET_IF_ERR_MSG(!cfg.isKey("pus_version"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_version");
  RET_IF_ERR_MSG(!cfg.isKey("pus_service_type"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_service_type");
  RET_IF_ERR_MSG(!cfg.isKey("pus_service_sub_type"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_service_sub_type");
  RET_IF_ERR_MSG(!cfg.isKey("pus_source_id"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_source_id");
  RET_IF_ERR_MSG(!cfg.isKey("pus_time_code"), CCSDS::ErrorCode::CONFIG_FILE_ERROR,"Config: Missing string field: pus_time_code");

  ASSIGN_OR_PRINT(version, cfg.get<int>("pus_version"));
  ASSIGN_OR_PRINT(serviceType, cfg.get<int>("pus_service_type"));
  ASSIGN_OR_PRINT(serviceSubType,cfg.get< int>("pus_service_sub_type"));
  ASSIGN_OR_PRINT(sourceId,cfg.get<int>("pus_source_id"));
  ASSIGN_OR_PRINT(timeCode,cfg.get<std::vector<uint8_t>>("pus_time_code"));

  m_version = version & 0x7;
  m_serviceType = serviceType & 0xFF;
  m_serviceSubType = serviceSubType & 0xFF;
  m_sourceID = sourceId & 0xFF;
  m_timeCode = timeCode;

  return true;
}
