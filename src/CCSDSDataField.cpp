#include "CCSDSDataField.h"
#include <CCSDSSecondaryHeaderFactory.h>
#include <iostream>
#include <utility>

std::vector<uint8_t> CCSDS::DataField::serialize() {
  update();
  const auto &dataFieldHeader = getDataFieldHeaderBytes();
  std::vector<uint8_t> fullData{};
  if (!dataFieldHeader.empty()) {
    fullData.insert(fullData.end(), dataFieldHeader.begin(), dataFieldHeader.end());
  }
  fullData.insert(fullData.end(), m_applicationData.begin(), m_applicationData.end());
  return fullData;
}

std::vector<uint8_t> CCSDS::DataField::getApplicationData() {
  return m_applicationData;
}

uint16_t CCSDS::DataField::getDataFieldAvailableBytesSize() const {
  return m_dataPacketSize - getDataFieldUsedBytesSize();
}

uint16_t CCSDS::DataField::getDataFieldAbsoluteBytesSize() const {
  return m_dataPacketSize;
}

uint16_t CCSDS::DataField::getApplicationDataBytesSize() const {
  return m_applicationData.size();
}


uint16_t CCSDS::DataField::getDataFieldUsedBytesSize() const {
  if (!getDataFieldHeaderFlag()) {
    return m_applicationData.size();
  }
  if (m_secondaryHeader != nullptr) {
    return m_applicationData.size() + m_secondaryHeader->getSize();
  }
  return 0;
}

std::shared_ptr<CCSDS::SecondaryHeaderAbstract> CCSDS::DataField::getSecondaryHeader() {
  update();
  return m_secondaryHeader;
}

void CCSDS::DataField::update() {
  if (!m_dataFieldHeaderUpdated && m_enableDataFieldUpdate) {
    if (m_secondaryHeaderFactory.typeIsRegistered(m_dataFieldHeaderType)) {
      m_secondaryHeader->update(this);
    }
    m_dataFieldHeaderUpdated = true;
  }
}

CCSDS::ResultBool CCSDS::DataField::setApplicationData(const uint8_t *pData, const size_t &sizeData) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Application data is nullptr");
  RET_IF_ERR_MSG(sizeData < 1, ErrorCode::INVALID_APPLICATION_DATA, "Application data size cannot be < 1");
  RET_IF_ERR_MSG(sizeData > getDataFieldAvailableBytesSize(), ErrorCode::INVALID_APPLICATION_DATA,
                 "Application data field exceeds available size");

  if (!m_applicationData.empty()) {
    std::cerr << "[ CCSDS Data ] Warning: Data field is not empty, it has been overwritten." << std::endl;
  }
  m_applicationData.assign(pData, pData + sizeData);
  m_dataFieldHeaderUpdated = false;
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setApplicationData(const std::vector<uint8_t> &applicationData) {
  RET_IF_ERR_MSG(applicationData.size() > getDataFieldAvailableBytesSize(), ErrorCode::INVALID_APPLICATION_DATA,
                 "Application data field exceeds available size.");
  m_applicationData = applicationData;
  m_dataFieldHeaderUpdated = false;
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const uint8_t *pData, const size_t &sizeData) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Secondary header data is nullptr");
  RET_IF_ERR_MSG(sizeData < 1, ErrorCode::INVALID_SECONDARY_HEADER_DATA, "Secondary header data size cannot be < 1");
  RET_IF_ERR_MSG(sizeData > getDataFieldAvailableBytesSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size.");

  std::vector<uint8_t> data;
  data.assign(pData, pData + sizeData);
  FORWARD_RESULT( setDataFieldHeader(data) );
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const uint8_t *pData, const size_t &sizeData,
                                                       const std::string &pType) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Secondary header data is nullptr");

  RET_IF_ERR_MSG(!m_secondaryHeaderFactory.typeIsRegistered(pType), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header type is not registered: " + pType);

  if (m_secondaryHeaderFactory.typeIsRegistered(pType)) {
    std::vector<uint8_t> data;
    data.assign(pData, pData + sizeData);
    FORWARD_RESULT(setDataFieldHeader(data,pType));
  } else {
    FORWARD_RESULT(setDataFieldHeader(pData, sizeData));
    m_dataFieldHeaderType = pType;
  }
  m_dataFieldHeaderUpdated = false;
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const std::vector<uint8_t> &data,
                                                       const std::string &pType) {
  RET_IF_ERR_MSG(data.size() > getDataFieldAvailableBytesSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");
  RET_IF_ERR_MSG(!m_secondaryHeaderFactory.typeIsRegistered(pType), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Secondary header type is not registered: " + pType);

  auto header = m_secondaryHeaderFactory.create(pType);

  if (pType != "PusC") {
    RET_IF_ERR_MSG(data.size() != header->getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                     "Secondary header data size mismatch for type: " + pType);
  }

  m_secondaryHeader = std::move(header);
  FORWARD_RESULT(m_secondaryHeader->deserialize(data));

  m_dataFieldHeaderType = pType;

  m_dataFieldHeaderUpdated = false;
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const std::vector<uint8_t> &dataFieldHeader) {
  RET_IF_ERR_MSG(dataFieldHeader.size() > getDataFieldAvailableBytesSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");

  BufferHeader header;
  m_secondaryHeader = std::make_shared<BufferHeader>(dataFieldHeader);
  FORWARD_RESULT(  m_secondaryHeader->deserialize(dataFieldHeader) );

  m_dataFieldHeaderType = m_secondaryHeader->getType(); ;
  m_dataFieldHeaderUpdated = false;
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const Config& cfg) {
  RET_IF_ERR_MSG(!cfg.isKey("secondary_header_type"), ErrorCode::CONFIG_FILE_ERROR,
                     "Config: Missing string field: secondary_header_type");
  std::string type{};
  ASSIGN_OR_PRINT(type, cfg.get<std::string>("secondary_header_type"));
  RET_IF_ERR_MSG(!m_secondaryHeaderFactory.typeIsRegistered(type), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Secondary header type is not registered: " + type);

  m_secondaryHeader = m_secondaryHeaderFactory.create(type);
  RET_IF_ERR_MSG(!m_secondaryHeader, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Failed to create secondary header of type: " + type);
  m_secondaryHeader->loadFromConfig(cfg);
  m_dataFieldHeaderType = m_secondaryHeader->getType();
  return true;
}

void CCSDS::DataField::setDataFieldHeader(std::shared_ptr<SecondaryHeaderAbstract> header) {
  m_secondaryHeader = std::move(header);
  m_dataFieldHeaderType = m_secondaryHeader->getType();
  m_dataFieldHeaderUpdated = false;
}

void CCSDS::DataField::setDataPacketSize(const uint16_t &value) { m_dataPacketSize = value; }

std::vector<uint8_t> CCSDS::DataField::getDataFieldHeaderBytes() {
  update();
  if (m_secondaryHeader) {
    return m_secondaryHeader->serialize();
  }
  return {};
}



