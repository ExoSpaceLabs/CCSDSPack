// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSDataField.h"
#include <CCSDSSecondaryHeaderFactory.h>
#include <utility>

ccsds::ResultBuffer ccsds::DataField::serialize() {
  update();
  const auto secondaryHeader = static_cast<const DataField &>(*this).getSecondaryHeaderBytes();
  RET_IF_ERR_MSG(m_secondaryHeader && secondaryHeader.size() != m_secondaryHeader->getSize(),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header serialization failed or returned an unexpected size.");
  RET_IF_ERR_MSG(secondaryHeader.size() + m_applicationData.size() > m_dataPacketSize,
                 ErrorCode::INVALID_DATA,
                 "Serialized packet data field exceeds its configured capacity.");
  std::vector<std::uint8_t> fullData{};
  fullData.reserve(secondaryHeader.size() + m_applicationData.size());
  fullData.insert(fullData.end(), secondaryHeader.begin(), secondaryHeader.end());
  fullData.insert(fullData.end(), m_applicationData.begin(), m_applicationData.end());
  return fullData;
}

std::vector<std::uint8_t> ccsds::DataField::getApplicationData() {
  return static_cast<const DataField &>(*this).getApplicationData();
}

std::vector<std::uint8_t> ccsds::DataField::getApplicationData() const {
  return m_applicationData;
}

std::uint16_t ccsds::DataField::getDataFieldAvailableBytesSize() const {
  return static_cast<std::uint16_t>(m_dataPacketSize - getDataFieldUsedBytesSize());
}

std::uint16_t ccsds::DataField::getDataFieldAbsoluteBytesSize() const {
  return m_dataPacketSize;
}

std::uint16_t ccsds::DataField::getApplicationDataBytesSize() const {
  return static_cast<std::uint16_t>(m_applicationData.size());
}

std::uint16_t ccsds::DataField::getDataFieldUsedBytesSize() const {
  if (!m_secondaryHeader) return static_cast<std::uint16_t>(m_applicationData.size());
  return static_cast<std::uint16_t>(m_applicationData.size() + m_secondaryHeader->getSize());
}

std::shared_ptr<ccsds::SecondaryHeaderAbstract> ccsds::DataField::getSecondaryHeader() {
  return m_secondaryHeader;
}

std::shared_ptr<const ccsds::SecondaryHeaderAbstract> ccsds::DataField::getSecondaryHeader() const {
  return m_secondaryHeader;
}

void ccsds::DataField::update() {
  if (!m_secondaryHeaderUpdated && m_enableSecondaryHeaderUpdate) {
    if (m_secondaryHeader) m_secondaryHeader->update(this);
    m_secondaryHeaderUpdated = true;
  }
}

void ccsds::DataField::clearContent() {
  m_secondaryHeader.reset();
  m_applicationData.clear();
  m_secondaryHeaderType.clear();
  m_secondaryHeaderUpdated = false;
}

ccsds::ResultBool ccsds::DataField::setApplicationData(const std::uint8_t *pData,
                                                       const std::size_t &sizeData) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Application data is nullptr");
  RET_IF_ERR_MSG(sizeData < 1U, ErrorCode::INVALID_APPLICATION_DATA,
                 "Application data size cannot be less than one");
  const auto secondaryHeaderSize = m_secondaryHeader
                                     ? static_cast<std::size_t>(m_secondaryHeader->getSize()) : 0U;
  RET_IF_ERR_MSG(sizeData + secondaryHeaderSize > m_dataPacketSize,
                 ErrorCode::INVALID_APPLICATION_DATA,
                 "Application data field exceeds available size");
  m_applicationData.assign(pData, pData + sizeData);
  m_secondaryHeaderUpdated = false;
  return true;
}

ccsds::ResultBool ccsds::DataField::setApplicationData(
    const std::vector<std::uint8_t> &applicationData) {
  const auto secondaryHeaderSize = m_secondaryHeader
                                     ? static_cast<std::size_t>(m_secondaryHeader->getSize()) : 0U;
  RET_IF_ERR_MSG(applicationData.size() + secondaryHeaderSize > m_dataPacketSize,
                 ErrorCode::INVALID_APPLICATION_DATA,
                 "Application data field exceeds available size");
  m_applicationData = applicationData;
  m_secondaryHeaderUpdated = false;
  return true;
}

ccsds::ResultBool ccsds::DataField::setSecondaryHeader(const std::uint8_t *pData,
                                                       const std::size_t &sizeData) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Secondary header data is nullptr");
  RET_IF_ERR_MSG(sizeData < 1U, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data size cannot be less than one");
  RET_IF_ERR_MSG(sizeData + m_applicationData.size() > m_dataPacketSize,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");
  const std::vector<std::uint8_t> data(pData, pData + sizeData);
  return setSecondaryHeader(data);
}

ccsds::ResultBool ccsds::DataField::setSecondaryHeader(const std::uint8_t *pData,
                                                       const std::size_t &sizeData,
                                                       const std::string &pType) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Secondary header data is nullptr");
  const std::vector<std::uint8_t> data(pData, pData + sizeData);
  return setSecondaryHeader(data, pType);
}

ccsds::ResultBool ccsds::DataField::setSecondaryHeader(
    const std::vector<std::uint8_t> &data, const std::string &pType) {
  RET_IF_ERR_MSG(data.size() + m_applicationData.size() > m_dataPacketSize,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");
  std::shared_ptr<SecondaryHeaderAbstract> header;
  if (pus::SecondaryHeaderFactory::isPusSelector(pType)) {
    auto result = m_pusSecondaryHeaderFactory.create(pType);
    if (!result) return result.error();
    header = result.value();
  } else {
    RET_IF_ERR_MSG(!m_secondaryHeaderFactory.typeIsRegistered(pType),
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Secondary header type is not registered: " + pType);
    header = m_secondaryHeaderFactory.create(pType);
  }
  RET_IF_ERR_MSG(!header, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Failed to create secondary header of type: " + pType);
  if (!header->variableLength) {
    RET_IF_ERR_MSG(data.size() != header->getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Secondary header data size mismatch for type: " + pType);
  }
  FORWARD_RESULT(header->deserialize(data));
  m_secondaryHeader = std::move(header);
  m_secondaryHeaderType = pType;
  m_secondaryHeaderUpdated = false;
  return true;
}

ccsds::ResultBool ccsds::DataField::setSecondaryHeader(
    const std::vector<std::uint8_t> &secondaryHeader) {
  RET_IF_ERR_MSG(secondaryHeader.size() + m_applicationData.size() > m_dataPacketSize,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");
  auto header = std::make_shared<BufferHeader>(secondaryHeader);
  FORWARD_RESULT(header->deserialize(secondaryHeader));
  m_secondaryHeader = std::move(header);
  m_secondaryHeaderType = m_secondaryHeader->getType();
  m_secondaryHeaderUpdated = false;
  return true;
}

#ifndef CCSDS_MCU
ccsds::ResultBool ccsds::DataField::setSecondaryHeader(const ccsds::Config &cfg) {
  RET_IF_ERR_MSG(!cfg.isKey("secondary_header_type"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing string field: secondary_header_type");
  std::string type{};
  ASSIGN_CP(type, cfg.get<std::string>("secondary_header_type"));
  RET_IF_ERR_MSG(type == "PusA" || type == "PusB" || type == "PusC",
                 ErrorCode::CONFIG_FILE_ERROR,
                 "Legacy PusA/PusB/PusC selectors were removed in v2; use a canonical PUS selector.");
  std::shared_ptr<SecondaryHeaderAbstract> header;
  if (pus::SecondaryHeaderFactory::isPusSelector(type)) {
    auto result = m_pusSecondaryHeaderFactory.create(type);
    if (!result) return result.error();
    header = result.value();
  } else {
    RET_IF_ERR_MSG(!m_secondaryHeaderFactory.typeIsRegistered(type),
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Secondary header type is not registered: " + type);
    header = m_secondaryHeaderFactory.create(type);
  }
  RET_IF_ERR_MSG(!header, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Failed to create secondary header of type: " + type);
  FORWARD_RESULT(header->loadFromConfig(cfg));
  m_secondaryHeader = std::move(header);
  m_secondaryHeaderType = m_secondaryHeader->getType();
  m_secondaryHeaderUpdated = false;
  return true;
}
#endif

ccsds::ResultBool ccsds::DataField::setSecondaryHeader(
    std::shared_ptr<SecondaryHeaderAbstract> header) {
  if (header) {
    RET_IF_ERR_MSG(static_cast<std::size_t>(header->getSize()) + m_applicationData.size()
                     > m_dataPacketSize,
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Secondary header exceeds available data-field capacity.");
    RET_IF_ERR_MSG(!header->isPusHeader()
                   && pus::SecondaryHeaderFactory::isPusSelector(header->getType()),
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Custom secondary headers cannot use the reserved PUS: namespace.");
  }
  m_secondaryHeader = std::move(header);
  m_secondaryHeaderType = m_secondaryHeader ? m_secondaryHeader->getType() : std::string{};
  m_secondaryHeaderUpdated = false;
  return true;
}

void ccsds::DataField::setDataPacketSize(const std::uint16_t &value) {
  m_dataPacketSize = value;
}

std::vector<std::uint8_t> ccsds::DataField::getSecondaryHeaderBytes() {
  return static_cast<const DataField &>(*this).getSecondaryHeaderBytes();
}

std::vector<std::uint8_t> ccsds::DataField::getSecondaryHeaderBytes() const {
  return m_secondaryHeader ? m_secondaryHeader->serialize() : std::vector<std::uint8_t>{};
}
