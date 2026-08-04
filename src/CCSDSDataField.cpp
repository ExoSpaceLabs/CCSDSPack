// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSDataField.h"
#include <CCSDSSecondaryHeaderFactory.h>
#include <utility>

CCSDS::ResultBuffer CCSDS::DataField::serialize() {
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
  if (!m_secondaryHeaderUpdated && m_enableSecondaryHeaderUpdate) {
    if (m_secondaryHeader) m_secondaryHeader->update(this);
    m_secondaryHeaderUpdated = true;
  }
}

CCSDS::ResultBool CCSDS::DataField::setMissionProfile(const MissionProfile &profile) {
  FORWARD_RESULT(validateMissionProfile(profile));
  if (m_secondaryHeader) {
    if (m_secondaryHeader->isPusHeader()) {
      RET_IF_ERR_MSG(!m_secondaryHeader->matchesMissionProfile(profile),
                     ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                     "Installed PUS secondary header does not match the new mission profile.");
    } else {
      RET_IF_ERR_MSG(profile.pusEnabled, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                     "A PUS mission profile cannot retain a custom secondary header.");
    }
  }
  m_missionProfile = profile;
  m_secondaryHeaderUpdated = false;
  return true;
}

void CCSDS::DataField::clearContent() {
  m_secondaryHeader.reset();
  m_applicationData.clear();
  m_secondaryHeaderType.clear();
  m_secondaryHeaderUpdated = false;
}

CCSDS::ResultBool CCSDS::DataField::setApplicationData(const std::uint8_t *pData,
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

CCSDS::ResultBool CCSDS::DataField::setApplicationData(
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

CCSDS::ResultBool CCSDS::DataField::setSecondaryHeader(const std::uint8_t *pData,
                                                       const std::size_t &sizeData) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Secondary header data is nullptr");
  RET_IF_ERR_MSG(sizeData < 1U, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data size cannot be less than one");
  RET_IF_ERR_MSG(sizeData + m_applicationData.size() > m_dataPacketSize,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");

  const std::vector<std::uint8_t> data(pData, pData + sizeData);
  FORWARD_RESULT(setSecondaryHeader(data));
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setSecondaryHeader(const std::uint8_t *pData,
                                                       const std::size_t &sizeData,
                                                       const std::string &pType) {
  RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER, "Secondary header data is nullptr");
  const std::vector<std::uint8_t> data(pData, pData + sizeData);
  FORWARD_RESULT(setSecondaryHeader(data, pType));
  return true;
}

CCSDS::ResultBool CCSDS::DataField::setSecondaryHeader(
    const std::vector<std::uint8_t> &data, const std::string &pType) {
  RET_IF_ERR_MSG(data.size() + m_applicationData.size() > m_dataPacketSize,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Secondary header data exceeds available size");
  std::shared_ptr<SecondaryHeaderAbstract> header;
  if (PusSecondaryHeaderFactory::isPusSelector(pType)) {
    auto result = m_pusSecondaryHeaderFactory.create(pType, m_missionProfile);
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

CCSDS::ResultBool CCSDS::DataField::setSecondaryHeader(
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
CCSDS::ResultBool CCSDS::DataField::setSecondaryHeader(const Config &cfg) {
  RET_IF_ERR_MSG(!cfg.isKey("secondary_header_type"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing string field: secondary_header_type");
  std::string type{};
  ASSIGN_CP(type, cfg.get<std::string>("secondary_header_type"));
  RET_IF_ERR_MSG(type == "PusA" || type == "PusB" || type == "PusC",
                 ErrorCode::CONFIG_FILE_ERROR,
                 "Legacy PusA/PusB/PusC selectors were removed in v2; use an explicit PUS profile and canonical selector.");
  std::shared_ptr<SecondaryHeaderAbstract> header;
  if (PusSecondaryHeaderFactory::isPusSelector(type)) {
    auto result = m_pusSecondaryHeaderFactory.create(type, m_missionProfile);
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

CCSDS::ResultBool CCSDS::DataField::setSecondaryHeader(
    std::shared_ptr<SecondaryHeaderAbstract> header) {
  if (header) {
    RET_IF_ERR_MSG(static_cast<std::size_t>(header->getSize()) + m_applicationData.size()
                     > m_dataPacketSize,
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Secondary header exceeds available data-field capacity.");
    if (header->isPusHeader()) {
      RET_IF_ERR_MSG(!m_missionProfile.pusEnabled
                     || !header->matchesMissionProfile(m_missionProfile),
                     ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                     "PUS secondary header does not match the active mission profile.");
    } else {
      RET_IF_ERR_MSG(PusSecondaryHeaderFactory::isPusSelector(header->getType()),
                     ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                     "Custom secondary headers cannot use the reserved PUS: namespace.");
      RET_IF_ERR_MSG(m_missionProfile.pusEnabled, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                     "A PUS mission profile requires a standards-defined PUS secondary header.");
    }
  }
  m_secondaryHeader = std::move(header);
  m_secondaryHeaderType = m_secondaryHeader ? m_secondaryHeader->getType() : std::string{};
  m_secondaryHeaderUpdated = false;
  return true;
}

void CCSDS::DataField::setDataPacketSize(const std::uint16_t &value) {
  m_dataPacketSize = value;
}

std::vector<std::uint8_t> CCSDS::DataField::getSecondaryHeaderBytes() {
  return static_cast<const DataField &>(*this).getSecondaryHeaderBytes();
}

std::vector<std::uint8_t> CCSDS::DataField::getSecondaryHeaderBytes() const {
  return m_secondaryHeader ? m_secondaryHeader->serialize() : std::vector<std::uint8_t>{};
}
