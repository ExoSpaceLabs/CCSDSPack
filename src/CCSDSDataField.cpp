// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSDataField.h"
#include <CCSDSSecondaryHeaderFactory.h>
#include <utility>

std::vector<std::uint8_t> CCSDS::DataField::serialize() {
  update();
  const auto dataFieldHeader = static_cast<const DataField &>(*this).getDataFieldHeaderBytes();
  std::vector<std::uint8_t> fullData{};
  fullData.reserve(dataFieldHeader.size() + m_applicationData.size());
  fullData.insert(fullData.end(), dataFieldHeader.begin(), dataFieldHeader.end());
  fullData.insert(fullData.end(), m_applicationData.begin(), m_applicationData.end());
  return fullData;
}

std::vector<std::uint8_t> CCSDS::DataField::getApplicationData() {
  return static_cast<const DataField &>(*this).getApplicationData();
}

std::vector<std::uint8_t> CCSDS::DataField::getApplicationData() const {
  return m_applicationData;
}

std::uint16_t CCSDS::DataField::getDataFieldAvailableBytesSize() const {
  return static_cast<std::uint16_t>(m_dataPacketSize - getDataFieldUsedBytesSize());
}

std::uint16_t CCSDS::DataField::getDataFieldAbsoluteBytesSize() const {
  return m_dataPacketSize;
}

std::uint16_t CCSDS::DataField::getApplicationDataBytesSize() const {
  return static_cast<std::uint16_t>(m_applicationData.size());
}

std::uint16_t CCSDS::DataField::getDataFieldUsedBytesSize() const {
  if (!m_secondaryHeader) {
    return static_cast<std::uint16_t>(m_applicationData.size());
  }
  return static_cast<std::uint16_t>(m_applicationData.size() + m_secondaryHeader->getSize());
}

std::shared_ptr<CCSDS::SecondaryHeaderAbstract> CCSDS::DataField::getSecondaryHeader() {
  return m_secondaryHeader;
}

std::shared_ptr<const CCSDS::SecondaryHeaderAbstract> CCSDS::DataField::getSecondaryHeader() const {
  return m_secondaryHeader;
}

void CCSDS::DataField::update() {
  if (!m_dataFieldHeaderUpdated && m_enableDataFieldUpdate) {
    if (m_secondaryHeader && m_secondaryHeaderFactory.typeIsRegistered(m_dataFieldHeaderType)) {
      m_secondaryHeader->update(this);
    }
    m_dataFieldHeaderUpdated = true;
  }
}

void CCSDS::DataField::clearContent() {
  m_secondaryHeader.reset();
  m_applicationData.clear();
  m_dataFieldHeaderType.clear();
  m_dataFieldHeaderUpdated = false;
}

CCSDS::ResultBool CCSDS::DataField::setApplicationData(const std::uint8_t *pData,
                                                       const std::size_t &sizeData) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Application data is nullptr");
  RET_IF_ERR_MSG(sizeData < 1U, ErrorCode::INVALID_APPLICATION_DATA,
                 "Application data size cannot be less than one");
  RET_IF_ERR_MSG(sizeData > getDataFieldAvailableBytesSize(), ErrorCode::INVALID_APPLICATION_DATA,
                 "Application data field exceeds available size");

  m_applicationData.assign(pData, pData + sizeData);
  m_dataFieldHeaderUpdated = false;
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setApplicationData(
    const std::vector<std::uint8_t> &applicationData) {
  RET_IF_ERR_MSG(applicationData.size() > getDataFieldAvailableBytesSize(),
                 ErrorCode::INVALID_APPLICATION_DATA,
                 "Application data field exceeds available size");
  m_applicationData = applicationData;
  m_dataFieldHeaderUpdated = false;
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const std::uint8_t *pData,
                                                       const std::size_t &sizeData) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Secondary header data is nullptr");
  RET_IF_ERR_MSG(sizeData < 1U, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data size cannot be less than one");
  RET_IF_ERR_MSG(sizeData > getDataFieldAvailableBytesSize(),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");

  const std::vector<std::uint8_t> data(pData, pData + sizeData);
  FORWARD_RESULT(setDataFieldHeader(data));
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const std::uint8_t *pData,
                                                       const std::size_t &sizeData,
                                                       const std::string &pType) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Secondary header data is nullptr");
  RET_IF_ERR_MSG(!m_secondaryHeaderFactory.typeIsRegistered(pType),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header type is not registered: " + pType);

  const std::vector<std::uint8_t> data(pData, pData + sizeData);
  FORWARD_RESULT(setDataFieldHeader(data, pType));
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(
    const std::vector<std::uint8_t> &data, const std::string &pType) {
  RET_IF_ERR_MSG(data.size() > getDataFieldAvailableBytesSize(),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");
  RET_IF_ERR_MSG(!m_secondaryHeaderFactory.typeIsRegistered(pType),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header type is not registered: " + pType);

  auto header = m_secondaryHeaderFactory.create(pType);
  RET_IF_ERR_MSG(!header, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Failed to create secondary header of type: " + pType);
  if (!header->variableLength) {
    RET_IF_ERR_MSG(data.size() != header->getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Secondary header data size mismatch for type: " + pType);
  }

  FORWARD_RESULT(header->deserialize(data));
  m_secondaryHeader = std::move(header);
  m_dataFieldHeaderType = pType;
  m_dataFieldHeaderUpdated = false;
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(
    const std::vector<std::uint8_t> &dataFieldHeader) {
  RET_IF_ERR_MSG(dataFieldHeader.size() > getDataFieldAvailableBytesSize(),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");

  auto header = std::make_shared<BufferHeader>(dataFieldHeader);
  FORWARD_RESULT(header->deserialize(dataFieldHeader));
  m_secondaryHeader = std::move(header);
  m_dataFieldHeaderType = m_secondaryHeader->getType();
  m_dataFieldHeaderUpdated = false;
  return true;
}

#ifndef CCSDS_MCU
CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const Config &cfg) {
  RET_IF_ERR_MSG(!cfg.isKey("secondary_header_type"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing string field: secondary_header_type");
  std::string type{};
  ASSIGN_CP(type, cfg.get<std::string>("secondary_header_type"));
  RET_IF_ERR_MSG(!m_secondaryHeaderFactory.typeIsRegistered(type),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header type is not registered: " + type);

  auto header = m_secondaryHeaderFactory.create(type);
  RET_IF_ERR_MSG(!header, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Failed to create secondary header of type: " + type);
  FORWARD_RESULT(header->loadFromConfig(cfg));
  m_secondaryHeader = std::move(header);
  m_dataFieldHeaderType = m_secondaryHeader->getType();
  m_dataFieldHeaderUpdated = false;
  return true;
}
#endif

void CCSDS::DataField::setDataFieldHeader(std::shared_ptr<SecondaryHeaderAbstract> header) {
  m_secondaryHeader = std::move(header);
  m_dataFieldHeaderType = m_secondaryHeader ? m_secondaryHeader->getType() : std::string{};
  m_dataFieldHeaderUpdated = false;
}

void CCSDS::DataField::setDataPacketSize(const std::uint16_t &value) {
  m_dataPacketSize = value;
}

std::vector<std::uint8_t> CCSDS::DataField::getDataFieldHeaderBytes() {
  return static_cast<const DataField &>(*this).getDataFieldHeaderBytes();
}

std::vector<std::uint8_t> CCSDS::DataField::getDataFieldHeaderBytes() const {
  return m_secondaryHeader ? m_secondaryHeader->serialize() : std::vector<std::uint8_t>{};
}
